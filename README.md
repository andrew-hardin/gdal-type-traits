# GDAL Type Traits

This module allows you to convert a template type to a GDALDataType at compile-time.

Q: Why would someone want this?

A: This module provides syntactic sugar for easily writing clean, generic, raster code
that interacts with the GDAL library. Here are a couple example uses:

```C++
// Generic row major matrix with RasterIO functions.
template<typename T>
class RowMajorMatrix<T> {
    // Determine the GDAL data type for the template argument.
    static const GDALDataType kRasterType = gdaltypetraits::Convert<T>::value;

    void Write() {
       // Provide the template argument as part of a RasterIO operation.
       RasterIO(...args..., kRasterType);
    }
}
```

```C++
template<typename T>
void DoSomething(T* array, size_t length) {

  // Squak if the user tries to use a type that doesn't have a GDALDataType equivalent
  // e.g. std::string
  static_assert(
    gdaltypetraits::Convert<T>::value != GDALDataType::GDT_Unknown
    "The template argument can't be converted to a GDAL data type.");

  // The idea of checking for GDT_Unknown can be extended into some interesting template metaprogramming...
}
```
