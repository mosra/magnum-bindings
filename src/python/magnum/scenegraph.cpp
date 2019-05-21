/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include <pybind11/pybind11.h>
#include <Corrade/Utility/FormatStl.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/MatrixTransformation2D.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>

#include "magnum/bootstrap.h"

/* This is a variant of https://github.com/pybind/pybind11/issues/1389. If the
   object has a parent, its refcount gets increased in order to avoid it being
   deleted by Python too soon. The refcount gets decreased when the parent is
   removed again or the parent gets deleted. I thought this would be doable
   inside py::init() as

    .def(py::init([](SceneGraph::Scene<Transformation>* parent) {
        auto self = new PyObject<Transformation>{parent};
        if(parent) py::cast(self).inc_ref();
        return self;
    }))

   but FOR SOME REASON py::cast(self) inside py::init() returns a different
   underlying PyObject pointer, so it only leads to crashes. */
namespace magnum { namespace {

template<class T> struct SceneGraphObjectHolder: std::unique_ptr<T> {
    explicit SceneGraphObjectHolder(T* object): std::unique_ptr<T>{object} {
        CORRADE_INTERNAL_ASSERT(object);
        if(object->parent()) py::cast(object).inc_ref();
    }
};

template<class T> struct SceneGraphFeatureHolder: std::unique_ptr<T> {
    explicit SceneGraphFeatureHolder(T* object): std::unique_ptr<T>{object} {
        CORRADE_INTERNAL_ASSERT(object);
        py::cast(object).inc_ref();
    }
};

}}

PYBIND11_DECLARE_HOLDER_TYPE(T, magnum::SceneGraphObjectHolder<T>)
PYBIND11_DECLARE_HOLDER_TYPE(T, magnum::SceneGraphFeatureHolder<T>)

namespace magnum { namespace {

template<class Transformation> class PyObject: public SceneGraph::Object<Transformation> {
    public:
        PyObject(SceneGraph::Object<Transformation>* parent): SceneGraph::Object<Transformation>{parent} {}

    private:
        void doErase() override {
            /* When deleting a parent, disconnect this from the parent instead
               of deleting it. Deletion is then handled by Python itself. */
            CORRADE_INTERNAL_ASSERT(SceneGraph::Object<Transformation>::parent());
            SceneGraph::Object<Transformation>::setParent(nullptr);
            py::cast(this).dec_ref();
        }
};

template<UnsignedInt dimensions, class T> struct PyDrawable: SceneGraph::Drawable<dimensions, T> {
    explicit PyDrawable(SceneGraph::AbstractObject<dimensions, T>& object, SceneGraph::DrawableGroup<dimensions, T>* drawables): SceneGraph::Drawable<dimensions, T>{object, drawables} {}

    void draw(const MatrixTypeFor<dimensions, T>& transformationMatrix, SceneGraph::Camera<dimensions, T>& camera) override {
        PYBIND11_OVERLOAD_PURE_NAME(
            void,
            PyDrawable,
            "draw",
            draw,
            transformationMatrix,
            camera
        );
    }

    void doErase() override {
        /* When deleting the holder object, disconnect this from that
           object instead of deleting it. This makes it rather useless, but
           better than having dangling memory or double deletion. This is of
           course not allowed by the C++ API due to private inheritance so we
           have to reinterpret self as the list instead. UGLY: */
        auto& listItem = reinterpret_cast<Containers::LinkedListItem<SceneGraph::AbstractFeature<dimensions, T>, SceneGraph::AbstractObject<dimensions, T>>&>(*this);

        CORRADE_INTERNAL_ASSERT(listItem.list());
        listItem.list()->features().cut(this);
        py::cast(this).dec_ref();
    }
};

template<UnsignedInt dimensions, class T> struct PyCamera: SceneGraph::Camera<dimensions, T> {
    explicit PyCamera(SceneGraph::AbstractObject<dimensions, T>& object): SceneGraph::Camera<dimensions, T>{object} {}

    void doErase() override {
        /* When deleting the holder object, disconnect this from that
           object instead of deleting it. This makes it rather useless, but
           better than having dangling memory or double deletion. This is of
           course not allowed by the C++ API due to private inheritance so we
           have to reinterpret self as the list instead. UGLY: */
        auto& listItem = reinterpret_cast<Containers::LinkedListItem<SceneGraph::AbstractFeature<dimensions, T>, SceneGraph::AbstractObject<dimensions, T>>&>(*this);

        CORRADE_INTERNAL_ASSERT(listItem.list());
        listItem.list()->features().cut(this);
        py::cast(this).dec_ref();
    }
};

template<class Transformation> void scene(py::class_<SceneGraph::Scene<Transformation>>& c) {
    c.def(py::init(), "Constructor");
}

template<UnsignedInt dimensions, class T> void abstractObject(py::class_<SceneGraph::AbstractObject<dimensions, T>, SceneGraphObjectHolder<SceneGraph::AbstractObject<dimensions, T>>>& c) {
    c
        /* Matrix transformation APIs */
        .def("transformation_matrix", &SceneGraph::AbstractObject<dimensions, T>::transformationMatrix,
            "Transformation matrix")
        .def("absolute_transformation_matrix", &SceneGraph::AbstractObject<dimensions, T>::absoluteTransformationMatrix,
             "Transformation matrix relative to the root object");
}

template<UnsignedInt dimensions, class T, class Transformation> void object(py::class_<PyObject<Transformation>, SceneGraph::AbstractObject<dimensions, T>, SceneGraphObjectHolder<PyObject<Transformation>>>& c) {
    c
        .def(py::init<PyObject<Transformation>*>(),
             "Constructor", py::arg("parent") = nullptr)
        .def(py::init<SceneGraph::Scene<Transformation>*>(),
            "Constructor", py::arg("parent") = nullptr)

        /* Properties */
        .def_property_readonly("scene", [](PyObject<Transformation>& self) {
            return static_cast<SceneGraph::Scene<Transformation>*>(self.scene());
        }, "Scene or None if the object is not a part of any scene")
        .def_property("parent", [](PyObject<Transformation>& self) {
            return static_cast<PyObject<Transformation>*>(self.parent());
        }, [](PyObject<Transformation>& self, py::object parentobj) {
            SceneGraph::Object<Transformation>* parent;
            if(py::isinstance<PyObject<Transformation>>(parentobj))
                parent = py::cast<PyObject<Transformation>*>(parentobj);
            else if(py::isinstance<SceneGraph::Scene<Transformation>>(parentobj))
                parent = py::cast<SceneGraph::Scene<Transformation>*>(parentobj);
            else if(py::isinstance<py::none>(parentobj))
                parent = nullptr;
            else throw py::type_error{Utility::formatString("expected Scene, Object or None, got {}", std::string(py::str{parentobj.get_type()}))};

            /* Decrease refcount if a parent is removed, increase it if a
               parent gets added */
            if(self.parent() && !parent) py::cast(&self).dec_ref();
            else if(!self.parent() && parent) py::cast(&self).inc_ref();

            self.setParent(parent);
        }, "Parent object or None if this is the root object")

        /* Transformation APIs common to all implementations */
        .def_property("transformation",
            &PyObject<Transformation>::transformation,
            &PyObject<Transformation>::setTransformation,
            "Object transformation")
        .def("absolute_transformation", &PyObject<Transformation>::absoluteTransformation,
             "Transformation relative to the root object")
        .def("reset_transformation", [](PyObject<Transformation>& self) {
            self.resetTransformation();
        }, "Reset the transformation")
        .def("transform", [](PyObject<Transformation>& self, const typename Transformation::DataType& transformation) {
            self.transform(transformation);
        }, "Transform the object")
        .def("transform_local", [](PyObject<Transformation>& self, const typename Transformation::DataType& transformation) {
            self.transformLocal(transformation);
        }, "Transform the object as a local transformation")
        .def("translate", [](PyObject<Transformation>& self, const VectorTypeFor<dimensions, typename Transformation::DataType::Type>& vector) {
            self.translate(vector);
        }, "Translate the object")
        .def("translate_local", [](PyObject<Transformation>& self, const VectorTypeFor<dimensions, typename Transformation::DataType::Type>& vector) {
            self.translateLocal(vector);
        }, "Translate the object as a local transformation");
}

template<UnsignedInt dimensions, class T, class Transformation> void object2D(py::class_<PyObject<Transformation>, SceneGraph::AbstractObject<dimensions, T>,  SceneGraphObjectHolder<PyObject<Transformation>>>& c) {
    c
        .def("rotate", [](PyObject<Transformation>& self, const Radd angle) {
            self.rotate(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle));
        }, "Rotate the object")
        .def("rotate_local", [](PyObject<Transformation>& self, const Radd angle) {
            self.rotateLocal(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle));
        }, "Rotate the object as a local transformation");
}

template<UnsignedInt dimensions, class T, class Transformation> void object3D(py::class_<PyObject<Transformation>, SceneGraph::AbstractObject<dimensions, T>, SceneGraphObjectHolder<PyObject<Transformation>>>& c) {
    c
        .def("rotate", [](PyObject<Transformation>& self, const Radd angle, const Math::Vector3<typename Transformation::DataType::Type>& normalizedAxis) {
            self.rotate(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle), normalizedAxis);
        }, "Rotate the object as a local transformation", py::arg("angle"), py::arg("normalized_axis"))
        .def("rotate_local", [](PyObject<Transformation>& self, const Radd angle, const Math::Vector3<typename Transformation::DataType::Type>& normalizedAxis) {
            self.rotateLocal(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle), normalizedAxis);
        }, "Rotate the object as a local transformation", py::arg("angle"), py::arg("normalized_axis"))
        .def("rotate_x", [](PyObject<Transformation>& self, const Radd angle) {
            self.rotateX(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle));
        }, "Rotate the object around X axis")
        .def("rotate_x_local", [](PyObject<Transformation>& self, const Radd angle) {
            self.rotateXLocal(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle));
        }, "Rotate the object around X axis as a local transformation")
        .def("rotate_y", [](PyObject<Transformation>& self, const Radd angle) {
            self.rotateY(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle));
        }, "Rotate the object around Y axis")
        .def("rotate_y_local", [](PyObject<Transformation>& self, const Radd angle) {
            self.rotateYLocal(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle));
        }, "Rotate the object around Y axis as a local transformation")
        .def("rotate_z", [](PyObject<Transformation>& self, const Radd angle) {
            self.rotateZ(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle));
        }, "Rotate the object around Z axis")
        .def("rotate_z_local", [](PyObject<Transformation>& self, const Radd angle) {
            self.rotateZLocal(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle));
        }, "Rotate the object around Z axis as a local transformation");
}

template<UnsignedInt dimensions, class T, class Transformation> void objectScale(py::class_<PyObject<Transformation>, SceneGraph::AbstractObject<dimensions, T>, SceneGraphObjectHolder<PyObject<Transformation>>>& c) {
    c
        .def("scale", [](PyObject<Transformation>& self, const VectorTypeFor<dimensions, typename Transformation::DataType::Type>& vector) {
            self.scale(vector);
        }, "Scale the object")
        .def("scale_local", [](PyObject<Transformation>& self, const VectorTypeFor<dimensions, typename Transformation::DataType::Type>& vector) {
            self.scaleLocal(vector);
        }, "Scale the object as a local transformation")
        .def("reflect", [](PyObject<Transformation>& self, const VectorTypeFor<dimensions, typename Transformation::DataType::Type>& vector) {
            self.reflect(vector);
        }, "Reflect the object")
        .def("reflect_local", [](PyObject<Transformation>& self, const VectorTypeFor<dimensions, typename Transformation::DataType::Type>& vector) {
            self.reflectLocal(vector);
        }, "Reflect the object as a local transformation");
}

template<class PyFeature, UnsignedInt dimensions, class Feature, class T> void featureGroup(py::class_<SceneGraph::FeatureGroup<dimensions, Feature, T>>& c) {
    c
        .def(py::init(), "Constructor")
        .def("__len__", &SceneGraph::FeatureGroup<dimensions, Feature, T>::size,
            "Count of features in the group")
        /* Get item. Fetching the already registered instance and returning
           that instead of wrapping the pointer again. Need to throw IndexError
           in order to allow iteration: https://docs.python.org/3/reference/datamodel.html#object.__getitem__ */
        .def("__getitem__", [](SceneGraph::FeatureGroup<dimensions, Feature, T>& self, std::size_t index) -> PyFeature& {
            if(index >= self.size()) throw pybind11::index_error{};
            return static_cast<PyFeature&>(self[index]);
        }, "Feature at given index")
        .def("add", [](SceneGraph::FeatureGroup<dimensions, Feature, T>& self, PyFeature& feature) {
            self.add(feature);
        }, "Add a feature to the group")
        .def("remove", [](SceneGraph::FeatureGroup<dimensions, Feature, T>& self, PyFeature& feature) {
            self.add(feature);
        }, "Remove a feature from the group");
}

template<UnsignedInt dimensions, class T, class Feature> void feature(py::class_<Feature, SceneGraphFeatureHolder<Feature>>& c) {
    c
        .def_property_readonly("object", [](Feature& self) -> SceneGraph::AbstractObject<dimensions, T>& {
            return self.object();
        }, "Object holding this feature");
}

template<UnsignedInt dimensions, class T> void drawable(py::class_<PyDrawable<dimensions, T>, SceneGraphFeatureHolder<PyDrawable<dimensions, T>>>& c) {
    c
        .def(py::init<SceneGraph::AbstractObject<dimensions, T>&, SceneGraph::DrawableGroup<dimensions, T>*>(),
            "Constructor", py::arg("object"), py::arg("drawables") = nullptr)
        .def_property_readonly("drawables", [](PyDrawable<dimensions, T>& self) {
            return self.drawables();
        }, "Group containing this drawable")
        .def("draw", [](PyDrawable<dimensions, T>& self, const MatrixTypeFor<dimensions, T>& transformationMatrix, PyCamera<dimensions, T>& camera) {
            self.draw(transformationMatrix, camera);
        }, "Draw the object using given camera", py::arg("transformation_matrix"), py::arg("camera"));
}

template<UnsignedInt dimensions, class T> void camera(py::class_<PyCamera<dimensions, T>, SceneGraphFeatureHolder<PyCamera<dimensions, T>>>& c) {
    c
        .def(py::init<SceneGraph::AbstractObject<dimensions, T>&>(),
            "Constructor", py::arg("object"))
        .def_property("aspect_ratio_policy", &SceneGraph::Camera<dimensions, T>::aspectRatioPolicy,
            /* Using a lambda because the setter has method chaining */
            [](PyCamera<dimensions, T>& self, SceneGraph::AspectRatioPolicy policy) {
                self.setAspectRatioPolicy(policy);
            }, "Aspect ratio policy")
        .def_property_readonly("camera_matrix", &SceneGraph::Camera<dimensions, T>::cameraMatrix,
            "Camera matrix")
        .def_property("projection_matrix", &SceneGraph::Camera<dimensions, T>::projectionMatrix,
            /* Using a lambda because the setter has method chaining */
            [](PyCamera<dimensions, T>& self, const MatrixTypeFor<dimensions, T>& matrix) {
                self.setProjectionMatrix(matrix);
            }, "Projection matrix")
        .def("projection_size", &SceneGraph::Camera<dimensions, T>::projectionSize,
            "Size of (near) XY plane in current projection")
        .def_property("viewport", &SceneGraph::Camera<dimensions, T>::viewport,
            &SceneGraph::Camera<dimensions, T>::setViewport,
            "Viewport size")
        .def("draw", static_cast<void(SceneGraph::Camera<dimensions, T>::*)(SceneGraph::DrawableGroup<dimensions, T>&)>(&SceneGraph::Camera<dimensions, T>::draw),
            "Draw");
}

void scenegraph(py::module& m) {
    /* Abstract objects. Returned from feature.object, so need to be registered
       as well. */
    {
        py::class_<SceneGraph::AbstractObject2D, SceneGraphObjectHolder<SceneGraph::AbstractObject2D>> abstractObject2D{m, "AbstractObject2D", "Base object for two-dimensional scenes"};
        py::class_<SceneGraph::AbstractObject3D, SceneGraphObjectHolder<SceneGraph::AbstractObject3D>> abstractObject3D{m, "AbstractObject3D", "Base object for three-dimensional scenes"};
        abstractObject(abstractObject2D);
        abstractObject(abstractObject3D);
    }

    /* 2D/3D matrix-based implementation */
    {
        py::module matrix = m.def_submodule("matrix");
        matrix.doc() = "General matrix-based scene graph implementation";

        py::class_<SceneGraph::Scene<SceneGraph::MatrixTransformation2D>> scene2D_{matrix, "Scene2D", "Two-dimensional scene with matrix-based transformation implementation"};
        scene(scene2D_);

        py::class_<SceneGraph::Scene<SceneGraph::MatrixTransformation3D>> scene3D_{matrix, "Scene3D", "Three-dimensional scene with matrix-based transformation implementation"};
        scene(scene3D_);

        py::class_<PyObject<SceneGraph::MatrixTransformation2D>, SceneGraph::AbstractObject2D, SceneGraphObjectHolder<PyObject<SceneGraph::MatrixTransformation2D>>> object2D_{matrix, "Object2D", "Two-dimensional object with matrix-based transformation implementation"};
        object(object2D_);
        object2D(object2D_);
        objectScale(object2D_);

        py::class_<PyObject<SceneGraph::MatrixTransformation3D>, SceneGraph::AbstractObject3D, SceneGraphObjectHolder<PyObject<SceneGraph::MatrixTransformation3D>>> object3D_{matrix, "Object3D", "Three-dimensional object with matrix-based transformation implementation"};
        object(object3D_);
        object3D(object3D_);
        objectScale(object3D_);
    }

    /* Drawables, camera */
    {
        py::enum_<SceneGraph::AspectRatioPolicy>{m, "AspectRatioPolicy", "Camera aspect ratio policy"}
            .value("NOT_PRESERVED", SceneGraph::AspectRatioPolicy::NotPreserved)
            .value("EXTEND", SceneGraph::AspectRatioPolicy::Extend)
            .value("CLIP", SceneGraph::AspectRatioPolicy::Clip);

        py::class_<SceneGraph::DrawableGroup2D> drawableGroup2D{m, "DrawableGroup2D", "Group of drawables for two-dimensional float scenes"};
        py::class_<SceneGraph::DrawableGroup3D> drawableGroup3D{m, "DrawableGroup3D", "Group of drawables for three-dimensional float scenes"};

        py::class_<PyDrawable<2, Float>, SceneGraphFeatureHolder<PyDrawable<2, Float>>> drawable2D{m, "Drawable2D", "Drawable for two-dimensional float scenes"};
        py::class_<PyDrawable<3, Float>, SceneGraphFeatureHolder<PyDrawable<3, Float>>> drawable3D{m, "Drawable3D", "Drawable for three-dimensional float scenes"};

        py::class_<PyCamera<2, Float>, SceneGraphFeatureHolder<PyCamera<2, Float>>> camera2D{m, "Camera2D", "Camera for two-dimensional float scenes"};
        py::class_<PyCamera<3, Float>, SceneGraphFeatureHolder<PyCamera<3, Float>>> camera3D{m, "Camera3D", "Camera for three-dimensional float scenes"};

        featureGroup<PyDrawable<2, Float>>(drawableGroup2D);
        featureGroup<PyDrawable<3, Float>>(drawableGroup3D);
        feature<2, Float>(drawable2D);
        feature<3, Float>(drawable3D);
        drawable(drawable2D);
        drawable(drawable3D);

        feature<2, Float>(camera2D);
        feature<3, Float>(camera3D);
        camera(camera2D);
        camera(camera3D);
    }
}

}}

PYBIND11_MODULE(scenegraph, m) {
    m.doc() = "Scene graph library";

    magnum::scenegraph(m);
}

