// Container.h: the fixed containers array<T>, matrix<T> and matrix3d<T>, arrays of one, two and three dimensions that
// are indexed from a minimum and check every index in operator(). The growable vector that belongs with them is
// TIndexedVector<T> (src/types/IndexedVector.h).

#pragma once

#include "common.h"
#include "compat/mdspan.h"
#include <memory>
#include <sstream>

// Each container owns its entries as a std::unique_ptr<T[]>, freed with reset() where the original had delete[] (the
// destructors), allocated with reset(new T[n]) (the same new-expression: entries of a type without a constructor stay
// uninitialised). The element types' destructors cannot throw (scalars, pointers and plain structures), so neither can
// the reset.

// array<T> is a fixed array indexed from min to min+d-1.
template <class T> class array {
public:
	array(const array &) = delete;
	array &operator=(const array &) = delete;
	array(int Min, int Max);
	~array() { entry.reset(); }
	T &operator()(int i);
	int low() { return min; }
	int high() { return min + d - 1; }

	int min;
	int d; // number of entries
	std::unique_ptr<T[]> entry;
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

// matrix<T> is a fixed two-dimensional array indexed from xmin/ymin; matrix3d<T> its three-dimensional counterpart. x
// is the fastest index. The destructors free the entries.
//
// The entries are indexed through a std::mdspan view over the owned storage (src/compat/mdspan.h): view() makes one
// from entry and the extents, layout_left, so view()[x, y] is entry[y * dx + x] and view()[x, y, z]
// entry[(z * dy + y) * dx + x] as in the original. operator() checks the indices first; a view does not.
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

	int xmin;
	int ymin;
	int dx;
	int dy;
	std::unique_ptr<T[]> entry;
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

	int xmin;
	int ymin;
	int zmin;
	int dx;
	int dy;
	int dz;
	std::unique_ptr<T[]> entry;
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
