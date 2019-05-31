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

#include "Magnum/SceneGraph/Python.h"

#include "magnum/bootstrap.h"

namespace magnum { namespace {

template<UnsignedInt dimensions, class T> struct PyDrawable: SceneGraph::PyFeature<SceneGraph::Drawable<dimensions, T>> {
    explicit PyDrawable(SceneGraph::AbstractObject<dimensions, T>& object, SceneGraph::DrawableGroup<dimensions, T>* drawables): SceneGraph::PyFeature<SceneGraph::Drawable<dimensions, T>>{object, drawables} {}

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
};

template<class Transformation> void scene(py::class_<SceneGraph::Scene<Transformation>>& c) {
    c.def(py::init(), "Constructor");
}

template<UnsignedInt dimensions, class T> void abstractObject(py::class_<SceneGraph::AbstractObject<dimensions, T>, SceneGraph::PyObjectHolder<SceneGraph::AbstractObject<dimensions, T>>>& c) {
    c
        /* Matrix transformation APIs */
        .def("transformation_matrix", &SceneGraph::AbstractObject<dimensions, T>::transformationMatrix,
            "Transformation matrix")
        .def("absolute_transformation_matrix", &SceneGraph::AbstractObject<dimensions, T>::absoluteTransformationMatrix,
             "Transformation matrix relative to the root object");
}

template<UnsignedInt dimensions, class T, class Transformation> void object(py::class_<SceneGraph::Object<Transformation>, SceneGraph::PyObject<SceneGraph::Object<Transformation>>, SceneGraph::AbstractObject<dimensions, T>, SceneGraph::PyObjectHolder<SceneGraph::Object<Transformation>>>& c) {
    c
        .def(py::init_alias<SceneGraph::PyObject<SceneGraph::Object<Transformation>>*>(),
             "Constructor", py::arg("parent") = nullptr)
        .def(py::init_alias<SceneGraph::Scene<Transformation>*>(),
            "Constructor", py::arg("parent") = nullptr)

        /* Properties */
        .def_property_readonly("scene", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self) {
            return static_cast<SceneGraph::Scene<Transformation>*>(self.scene());
        }, "Scene or None if the object is not a part of any scene")
        .def_property("parent", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self) {
            return static_cast<SceneGraph::PyObject<SceneGraph::Object<Transformation>>*>(self.parent());
        }, [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, py::object parentobj) {
            SceneGraph::Object<Transformation>* parent;
            if(py::isinstance<SceneGraph::PyObject<SceneGraph::Object<Transformation>>>(parentobj))
                parent = py::cast<SceneGraph::PyObject<SceneGraph::Object<Transformation>>*>(parentobj);
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
            &SceneGraph::PyObject<SceneGraph::Object<Transformation>>::transformation,
            &SceneGraph::PyObject<SceneGraph::Object<Transformation>>::setTransformation,
            "Object transformation")
        .def("absolute_transformation", &SceneGraph::PyObject<SceneGraph::Object<Transformation>>::absoluteTransformation,
             "Transformation relative to the root object")
        .def("reset_transformation", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self) {
            self.resetTransformation();
        }, "Reset the transformation")
        .def("transform", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, const typename Transformation::DataType& transformation) {
            self.transform(transformation);
        }, "Transform the object")
        .def("transform_local", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, const typename Transformation::DataType& transformation) {
            self.transformLocal(transformation);
        }, "Transform the object as a local transformation")
        .def("translate", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, const VectorTypeFor<dimensions, typename Transformation::DataType::Type>& vector) {
            self.translate(vector);
        }, "Translate the object")
        .def("translate_local", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, const VectorTypeFor<dimensions, typename Transformation::DataType::Type>& vector) {
            self.translateLocal(vector);
        }, "Translate the object as a local transformation");
}

template<UnsignedInt dimensions, class T, class Transformation> void object2D(py::class_<SceneGraph::Object<Transformation>, SceneGraph::PyObject<SceneGraph::Object<Transformation>>, SceneGraph::AbstractObject<dimensions, T>,  SceneGraph::PyObjectHolder<SceneGraph::Object<Transformation>>>& c) {
    c
        .def("rotate", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, const Radd angle) {
            self.rotate(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle));
        }, "Rotate the object")
        .def("rotate_local", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, const Radd angle) {
            self.rotateLocal(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle));
        }, "Rotate the object as a local transformation");
}

template<UnsignedInt dimensions, class T, class Transformation> void object3D(py::class_<SceneGraph::Object<Transformation>, SceneGraph::PyObject<SceneGraph::Object<Transformation>>, SceneGraph::AbstractObject<dimensions, T>, SceneGraph::PyObjectHolder<SceneGraph::Object<Transformation>>>& c) {
    c
        .def("rotate", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, const Radd angle, const Math::Vector3<typename Transformation::DataType::Type>& normalizedAxis) {
            self.rotate(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle), normalizedAxis);
        }, "Rotate the object as a local transformation", py::arg("angle"), py::arg("normalized_axis"))
        .def("rotate_local", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, const Radd angle, const Math::Vector3<typename Transformation::DataType::Type>& normalizedAxis) {
            self.rotateLocal(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle), normalizedAxis);
        }, "Rotate the object as a local transformation", py::arg("angle"), py::arg("normalized_axis"))
        .def("rotate_x", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, const Radd angle) {
            self.rotateX(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle));
        }, "Rotate the object around X axis")
        .def("rotate_x_local", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, const Radd angle) {
            self.rotateXLocal(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle));
        }, "Rotate the object around X axis as a local transformation")
        .def("rotate_y", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, const Radd angle) {
            self.rotateY(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle));
        }, "Rotate the object around Y axis")
        .def("rotate_y_local", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, const Radd angle) {
            self.rotateYLocal(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle));
        }, "Rotate the object around Y axis as a local transformation")
        .def("rotate_z", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, const Radd angle) {
            self.rotateZ(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle));
        }, "Rotate the object around Z axis")
        .def("rotate_z_local", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, const Radd angle) {
            self.rotateZLocal(static_cast<Math::Rad<typename Transformation::DataType::Type>>(angle));
        }, "Rotate the object around Z axis as a local transformation");
}

template<UnsignedInt dimensions, class T, class Transformation> void objectScale(py::class_<SceneGraph::Object<Transformation>, SceneGraph::PyObject<SceneGraph::Object<Transformation>>, SceneGraph::AbstractObject<dimensions, T>, SceneGraph::PyObjectHolder<SceneGraph::Object<Transformation>>>& c) {
    c
        .def("scale", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, const VectorTypeFor<dimensions, typename Transformation::DataType::Type>& vector) {
            self.scale(vector);
        }, "Scale the object")
        .def("scale_local", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, const VectorTypeFor<dimensions, typename Transformation::DataType::Type>& vector) {
            self.scaleLocal(vector);
        }, "Scale the object as a local transformation")
        .def("reflect", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, const VectorTypeFor<dimensions, typename Transformation::DataType::Type>& vector) {
            self.reflect(vector);
        }, "Reflect the object")
        .def("reflect_local", [](SceneGraph::PyObject<SceneGraph::Object<Transformation>>& self, const VectorTypeFor<dimensions, typename Transformation::DataType::Type>& vector) {
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

template<UnsignedInt dimensions, class T> void feature(py::class_<SceneGraph::AbstractFeature<dimensions, T>, SceneGraph::PyFeatureHolder<SceneGraph::AbstractFeature<dimensions, T>>>& c) {
    c
        .def_property_readonly("object", [](SceneGraph::AbstractFeature<dimensions, T>& self) -> SceneGraph::AbstractObject<dimensions, T>& {
            return self.object();
        }, "Object holding this feature");
}

template<UnsignedInt dimensions, class T> void drawable(py::class_<SceneGraph::Drawable<dimensions, T>, SceneGraph::AbstractFeature<dimensions, T>, PyDrawable<dimensions, T>, SceneGraph::PyFeatureHolder<SceneGraph::Drawable<dimensions, T>>>& c) {
    c
        .def(py::init_alias<SceneGraph::AbstractObject<dimensions, T>&, SceneGraph::DrawableGroup<dimensions, T>*>(),
            "Constructor", py::arg("object"), py::arg("drawables") = nullptr)
        .def_property_readonly("drawables", [](PyDrawable<dimensions, T>& self) {
            return self.drawables();
        }, "Group containing this drawable")
        .def("draw", [](PyDrawable<dimensions, T>& self, const MatrixTypeFor<dimensions, T>& transformationMatrix, SceneGraph::Camera<dimensions, T>& camera) {
            self.draw(transformationMatrix, camera);
        }, "Draw the object using given camera", py::arg("transformation_matrix"), py::arg("camera"));
}

template<UnsignedInt dimensions, class T> void camera(py::class_<SceneGraph::Camera<dimensions, T>, SceneGraph::AbstractFeature<dimensions, T>, SceneGraph::PyFeature<SceneGraph::Camera<dimensions, T>>, SceneGraph::PyFeatureHolder<SceneGraph::Camera<dimensions, T>>>& c) {
    c
        .def(py::init_alias<SceneGraph::AbstractObject<dimensions, T>&>(),
            "Constructor", py::arg("object"))
        .def_property("aspect_ratio_policy", &SceneGraph::Camera<dimensions, T>::aspectRatioPolicy,
            /* Using a lambda because the setter has method chaining */
            [](SceneGraph::Camera<dimensions, T>& self, SceneGraph::AspectRatioPolicy policy) {
                self.setAspectRatioPolicy(policy);
            }, "Aspect ratio policy")
        .def_property_readonly("camera_matrix", &SceneGraph::Camera<dimensions, T>::cameraMatrix,
            "Camera matrix")
        .def_property("projection_matrix", &SceneGraph::Camera<dimensions, T>::projectionMatrix,
            /* Using a lambda because the setter has method chaining */
            [](SceneGraph::Camera<dimensions, T>& self, const MatrixTypeFor<dimensions, T>& matrix) {
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
        py::class_<SceneGraph::AbstractObject2D, SceneGraph::PyObjectHolder<SceneGraph::AbstractObject2D>> abstractObject2D{m, "AbstractObject2D", "Base object for two-dimensional scenes"};
        py::class_<SceneGraph::AbstractObject3D, SceneGraph::PyObjectHolder<SceneGraph::AbstractObject3D>> abstractObject3D{m, "AbstractObject3D", "Base object for three-dimensional scenes"};
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

        py::class_<SceneGraph::Object<SceneGraph::MatrixTransformation2D>, SceneGraph::PyObject<SceneGraph::Object<SceneGraph::MatrixTransformation2D>>, SceneGraph::AbstractObject2D, SceneGraph::PyObjectHolder<SceneGraph::Object<SceneGraph::MatrixTransformation2D>>> object2D_{matrix, "Object2D", "Two-dimensional object with matrix-based transformation implementation"};
        object(object2D_);
        object2D(object2D_);
        objectScale(object2D_);

        py::class_<SceneGraph::Object<SceneGraph::MatrixTransformation3D>, SceneGraph::PyObject<SceneGraph::Object<SceneGraph::MatrixTransformation3D>>, SceneGraph::AbstractObject3D, SceneGraph::PyObjectHolder<SceneGraph::Object<SceneGraph::MatrixTransformation3D>>> object3D_{matrix, "Object3D", "Three-dimensional object with matrix-based transformation implementation"};
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

        py::class_<SceneGraph::AbstractFeature2D, SceneGraph::PyFeatureHolder<SceneGraph::AbstractFeature2D>> feature2D{m, "AbstractFeature2D", "Base for two-dimensional float features"};
        py::class_<SceneGraph::AbstractFeature3D, SceneGraph::PyFeatureHolder<SceneGraph::AbstractFeature3D>> feature3D{m, "AbstractFeature3D", "Base for three-dimensional float features"};
        feature(feature2D);
        feature(feature3D);

        py::class_<SceneGraph::Drawable2D, SceneGraph::AbstractFeature2D, PyDrawable<2, Float>, SceneGraph::PyFeatureHolder<SceneGraph::Drawable2D>> drawable2D{m, "Drawable2D", "Drawable for two-dimensional float scenes"};
        py::class_<SceneGraph::Drawable3D, SceneGraph::AbstractFeature3D, PyDrawable<3, Float>, SceneGraph::PyFeatureHolder<SceneGraph::Drawable3D>> drawable3D{m, "Drawable3D", "Drawable for three-dimensional float scenes"};

        py::class_<SceneGraph::Camera2D, SceneGraph::AbstractFeature2D, SceneGraph::PyFeature<SceneGraph::Camera2D>, SceneGraph::PyFeatureHolder<SceneGraph::Camera2D>> camera2D{m, "Camera2D", "Camera for two-dimensional float scenes"};
        py::class_<SceneGraph::Camera3D, SceneGraph::AbstractFeature3D, SceneGraph::PyFeature<SceneGraph::Camera3D>, SceneGraph::PyFeatureHolder<SceneGraph::Camera3D>> camera3D{m, "Camera3D", "Camera for three-dimensional float scenes"};

        featureGroup<PyDrawable<2, Float>>(drawableGroup2D);
        featureGroup<PyDrawable<3, Float>>(drawableGroup3D);
        drawable(drawable2D);
        drawable(drawable3D);

        camera(camera2D);
        camera(camera3D);
    }
}

}}

PYBIND11_MODULE(scenegraph, m) {
    m.doc() = "Scene graph library";

    magnum::scenegraph(m);
}

