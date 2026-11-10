// Container.h: CipSoft's container templates (c:\mud\tibiaplayer\Container.h).
//
// The assertion texts and line numbers come from the binary's
// instances; member names other than space and block are ours.
//
// The original file's growable vector<T> is TIndexedVector<T> of src/types/IndexedVector.h since
// Patella-Modern issue #99, the same semantics over a std::vector.

#pragma once

#include "common.h"
#include "compat/mdspan.h"
#include <memory>
#include <sstream>

// Each container owns its entries as a std::unique_ptr<T[]> (issue #58): a pointer at the original's offset, freed
// with reset() where the original had delete[] (the destructors), allocated with reset(new T[n])
// (the same new-expression: entries of a type without a constructor stay uninitialised). The element types'
// destructors cannot throw (scalars, pointers and plain structures), so neither can the reset.
static_assert(sizeof(std::unique_ptr<int[]>) == sizeof(int *) && alignof(std::unique_ptr<int[]>) == alignof(int *),
	"std::unique_ptr<T[]> is a pointer");

// array<T> is a fixed array indexed from min to min+d-1. The class name is
// ours; min, d, i, low() and high() are named in the assertions and in the
// error message of operator().
template <class T> class array {
public:
	array(const array &) = delete;
	array &operator=(const array &) = delete;
	array(int Min, int Max);
	// Inlined into Objects.cpp's ReleaseObjects (delete[] entry, then
	// operator delete on the array itself).
	~array() { entry.reset(); }
	T &operator()(int i);
	int low() { return min; }
	int high() { return min + d - 1; }

	int min;                    // +0x00
	int d;                      // +0x04, number of entries
	std::unique_ptr<T[]> entry; // +0x08
};

template <class T> array<T>::array(int Min, int Max)
{
	min = Min;
	d = Max - Min + 1;
	PATELLA_ASSERT_VALUES(d>0, d);
	entry.reset(new T[d]);
}

// The error's message is built in a stringstream.
template <class T> T &array<T>::operator()(int i)
{
	i -= min;
	if (i < 0 || i >= d) {
		std::stringstream s;
		s << "index out of range (" << "original i: " << i + min << ", i: " << i << ", min: " << min
		  << ", d: " << d << ")";
		TERROR(s.str().c_str());
	}
	return entry[i];
}

// matrix<T> is a fixed two-dimensional array indexed from xmin/ymin; matrix3d<T> its three-dimensional counterpart.
// The class names are ours; xmin, ymin, zmin, dx, dy, dz, x, y and z are named in the assertions and error messages.
// x is the fastest index. The destructors free the entries.
//
// The entries are indexed through a std::mdspan view over the owned storage since Patella-Modern issue #101
// (src/compat/mdspan.h; notes/decisions.md, "#101: the matrices and the automap layers as mdspan views"): view()
// makes one from entry and the extents, layout_left, so view()[x, y] is entry[y * dx + x] and view()[x, y, z]
// entry[(z * dy + y) * dx + x] as in the original. operator() checks the indices first; a view does not. The members
// before entry and entry itself stay as they were (the golden test compares them with the original's).
template <class T> class matrix {
public:
	matrix(const matrix &) = delete;
	matrix &operator=(const matrix &) = delete;
	matrix(int XMin, int XMax, int YMin, int YMax);
	~matrix() { entry.reset(); }
	T &operator()(int x, int y);

	// [x, y] from 0, x the fastest index.
	using TView = patella::TMdspan<T, patella::TDynamicExtents<int, 2>, patella::TLayoutLeft>;
	using TConstView = patella::TMdspan<const T, patella::TDynamicExtents<int, 2>, patella::TLayoutLeft>;
	TView view() { return TView(entry.get(), dx, dy); }
	[[nodiscard]] TConstView view() const { return TConstView(entry.get(), dx, dy); }

	int xmin;                   // +0x00
	int ymin;                   // +0x04
	int dx;                     // +0x08
	int dy;                     // +0x0c
	std::unique_ptr<T[]> entry; // +0x10
};

template <class T> matrix<T>::matrix(int XMin, int XMax, int YMin, int YMax)
{
	xmin = XMin;
	ymin = YMin;
	dx = XMax - XMin + 1;
	dy = YMax - YMin + 1;
	PATELLA_ASSERT_VALUES(dx>0, dx);
	PATELLA_ASSERT_VALUES(dy>0, dy);
	entry.reset(new T[dx * dy]);
}

template <class T> T &matrix<T>::operator()(int x, int y)
{
	x -= xmin;
	y -= ymin;
	if (x < 0 || x >= dx || y < 0 || y >= dy) {
		std::stringstream s;
		s << "index out of range (" << "original x: " << x + xmin << ", original y: " << y + ymin
		  << ", x: " << x << ", y: " << y << ", xmin: " << xmin << ", ymin: " << ymin << ", dx: " << dx
		  << ", dy: " << dy << ")";
		TERROR(s.str().c_str());
	}
	return view()[x, y];
}

template <class T> class matrix3d {
public:
	matrix3d(const matrix3d &) = delete;
	matrix3d &operator=(const matrix3d &) = delete;
	matrix3d(int XMin, int XMax, int YMin, int YMax, int ZMin, int ZMax);
	~matrix3d() { entry.reset(); }
	T &operator()(int x, int y, int z);

	// [x, y, z] from 0, x the fastest index, z the slowest.
	using TView = patella::TMdspan<T, patella::TDynamicExtents<int, 3>, patella::TLayoutLeft>;
	using TConstView = patella::TMdspan<const T, patella::TDynamicExtents<int, 3>, patella::TLayoutLeft>;
	TView view() { return TView(entry.get(), dx, dy, dz); }
	[[nodiscard]] TConstView view() const { return TConstView(entry.get(), dx, dy, dz); }

	int xmin;                   // +0x00
	int ymin;                   // +0x04
	int zmin;                   // +0x08
	int dx;                     // +0x0c
	int dy;                     // +0x10
	int dz;                     // +0x14
	std::unique_ptr<T[]> entry; // +0x18
};

template <class T> matrix3d<T>::matrix3d(int XMin, int XMax, int YMin, int YMax, int ZMin, int ZMax)
{
	xmin = XMin;
	ymin = YMin;
	zmin = ZMin;
	dx = XMax - XMin + 1;
	dy = YMax - YMin + 1;
	dz = ZMax - ZMin + 1;
	PATELLA_ASSERT_VALUES(dx>0, dx);
	PATELLA_ASSERT_VALUES(dy>0, dy);
	PATELLA_ASSERT_VALUES(dz>0, dz);
	entry.reset(new T[dx * dy * dz]);
}

// Unlike the other containers, the error names a bug number as its
// expression.
template <class T> T &matrix3d<T>::operator()(int x, int y, int z)
{
	x -= xmin;
	y -= ymin;
	z -= zmin;
	if (x < 0 || x >= dx || y < 0 || y >= dy || z < 0 || z >= dz) {
		std::stringstream s;
		s << "index out of range (" << "original x: " << x + xmin << ", original y: " << y + ymin
		  << ", original z: " << z + zmin << ", x: " << x << ", y: " << y << ", z: " << z
		  << ", xmin: " << xmin << ", ymin: " << ymin << ", zmin: " << zmin << ", dx: " << dx
		  << ", dy: " << dy << ", dz: " << dz << ")";
		throw TMYEXCEPTION(s.str().c_str(), "", 0, "", 0, "[bug0000013]");
	}
	return view()[x, y, z];
}
