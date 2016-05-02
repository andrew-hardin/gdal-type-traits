// MIT License
//
// Copyright (c) 2016 Andrew Hardin
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.


/*
Author: Andrew Hardin (andrew.hardin5+code@gmail.com)
Date: May 2, 2016


Purpose: Convert between a template type and a GDAL data type.

Why? Syntactic sugar + ease of writing generic raster code. This module
allows you to write templated objects that can communicate their 
basic type to GDAL. For example:

        template<typename T>
        class RowMajorMatrix<T> {
            static const GDALDataType kRasterType = gdaltypetraits::Convert<T>::value;

            void Write() {
               RasterIO(...args..., kRasterType);   
            }
        }

    Voila! We can now seamlessly do things like this:

            RowMajorMatrix<float>::Write();
                  --- and ---
            RowMajorMatrix<int>::Write();



Like Boost, the internals of this module are wrapped in an `internal` namespace.
The only object outside the internal namespace maps a template to a gdal data 
type:

    GDALDataType type = Convert<T>::value;

The structure within the internal namespace is a set of hierarchical templates.
Starting at the most generic level (ToGDAL<T>), the template type is specialized
until it reaches a GDALDataType. This hierarchy can handle floating point,
integral, boolean, and enumeration types.

Compile-time tests are included in a `test` namespace.
*/


#pragma once
#include <climits>
#include <type_traits>

#include "gdal.h"

namespace gdaltypetraits {
namespace internal {

static_assert(
    CHAR_BIT == 8,
    "This module is tailored toward systems where char == 8 bits. How old is this system?");


template<size_t FloatSize>
// Convert the size of a floating point number to a GDAL data type.
struct FloatToGDAL { static const GDALDataType value = GDALDataType::GDT_Unknown; };

template<>
struct FloatToGDAL<4> { static const GDALDataType value = GDALDataType::GDT_Float32; };

template<>
struct FloatToGDAL<8> { static const GDALDataType value = GDALDataType::GDT_Float64; };




template<size_t IntegralSize, bool IsSigned>
// Convert the size + signed property of a integral type to a GDAL data type.
struct IntegralToGDAL { static const GDALDataType value = GDALDataType::GDT_Unknown; };

template<>
struct IntegralToGDAL<1, false> { static const GDALDataType value = GDALDataType::GDT_Byte; };

template<>
struct IntegralToGDAL<1, true> { static const GDALDataType value = GDALDataType::GDT_Byte; };

template<>
struct IntegralToGDAL<2, false> { static const GDALDataType value = GDALDataType::GDT_UInt16; };

template<>
struct IntegralToGDAL<2, true> { static const GDALDataType value = GDALDataType::GDT_Int16; };

template<>
struct IntegralToGDAL<4, false> { static const GDALDataType value = GDALDataType::GDT_UInt32; };

template<>
struct IntegralToGDAL<4, true> { static const GDALDataType value = GDALDataType::GDT_Int32; };




// Convert a generic type to a a GDAL data type.
// The template arguments are used to determine which subcontractor
// template to use.
template<bool IsIntegral, bool IsFloat, bool IsEnum, typename T>
struct BuiltInToGDAL {
    static const GDALDataType value = GDALDataType::GDT_Unknown;
};


// Integral -> GDAL
template<typename T>
struct BuiltInToGDAL<true, false, false, T> {
    static const GDALDataType value = IntegralToGDAL<sizeof(T), std::is_signed<T>::value>::value;
};

// Float -> GDAL
template<typename T>
struct BuiltInToGDAL<false, true, false, T> {
    static const GDALDataType value = FloatToGDAL<sizeof(T)>::value;
};

// Enumeration -> Integral -> GDAL
template<typename T>
struct BuiltInToGDAL<false, false, true, T> {
    typedef typename std::underlying_type<T>::type UnderlyingType;
    
    static_assert(
        std::is_integral<UnderlyingType>::value,
        "The enumeration base type must be integral.");

    static const GDALDataType value = BuiltInToGDAL<true, false, false, UnderlyingType>::value;
};



// Template -> {Integral, Float, or Enumeration} -> GDAL
template<typename T>
struct Convert {
    static const GDALDataType value =
        BuiltInToGDAL<std::is_integral<T>::value, std::is_floating_point<T>::value, std::is_enum<T>::value, T>::value;
};

}  // namespace internal




// Convert a generic template to a GDAL data type.
// If the conversion fails (e.g. ToGDAL<std::string>), then GDT_Unknown is returned.
// Tip: this is a compile-time constant. To forbid GDT_Unknown, you can use a static_assert.
template<typename T>
struct Convert {

    // The template argument mapped to a GDAL data type.
    static const GDALDataType value = internal::Convert<T>::value;

    // Whether or not the template argument is a recognized GDAL data type.
    static const bool is_recognized = value != GDALDataType::GDT_Unknown;
};



// Compile-time tests to ensure that the template specialization tree is correct.
namespace test {

// Test all the basic types.
static_assert(Convert<int8_t>::value   == GDALDataType::GDT_Byte,    "Convert<T> test failed.");
static_assert(Convert<int16_t>::value  == GDALDataType::GDT_Int16,   "Convert<T> test failed.");
static_assert(Convert<int32_t>::value  == GDALDataType::GDT_Int32,   "Convert<T> test failed.");
static_assert(Convert<uint8_t>::value  == GDALDataType::GDT_Byte,    "Convert<T> test failed.");
static_assert(Convert<uint16_t>::value == GDALDataType::GDT_UInt16,  "Convert<T> test failed.");
static_assert(Convert<uint32_t>::value == GDALDataType::GDT_UInt32,  "Convert<T> test failed.");
static_assert(Convert<float_t>::value  == GDALDataType::GDT_Float32, "Convert<T> test failed.");
static_assert(Convert<double_t>::value == GDALDataType::GDT_Float64, "Convert<T> test failed.");

// Check that we generate Unknown when given a bad type.
static_assert(Convert<void*>::value    == GDALDataType::GDT_Unknown, "Convert<T> test failed.");
static_assert(!Convert<void*>::is_recognized, "Convert<T> test failed.");
static_assert(Convert<int8_t>::is_recognized, "Convert<T> test failed.");

// Test enumeration logic.
enum EnumInt8 :int8_t { Foo };
enum EnumUInt32 :uint32_t { Baz };
static_assert(Convert<EnumInt8>::value   == GDALDataType::GDT_Byte,   "Convert<T> test failed.");
static_assert(Convert<EnumUInt32>::value == GDALDataType::GDT_UInt32, "Convert<T> test failed.");

}  // namespace test
}  // namespace gdaltypetraits
