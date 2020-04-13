#pragma once

template<class T>
class Vector2D
{
public:
	Vector2D() : X(0), Y(0) {};
	Vector2D(T x, T y) : X(x), Y(y) {};

	
	Vector2D(const Vector2D<T> &other) : X(other.X), Y(other.Y) {}
	Vector2D<T> &operator=(const Vector2D<T> &other)
	{
		X = other.X;
		Y = other.Y;
		return *this;
	}

	bool operator==(const Vector2D<T> &other) const
	{
		return X == other.X && Y == other.Y;
	}
	bool operator!=(const Vector2D<T> &other) const
	{
		return X != other.X || Y != other.Y;
	}

#define MAKEOP(OP) \
	Vector2D<T> operator OP (const Vector2D<T> &other) const \
	{ return Vector2D<T>(X OP other.X, Y OP other.Y); } \
	Vector2D<T> operator OP (T other) const \
	{ return Vector2D<T>(X OP other, Y OP other); }

	MAKEOP(+);
	MAKEOP(-);
	MAKEOP(*);
	MAKEOP(/);

	/*Vector2D<T> min(const Vector2D &other) const
	{
		return Vector2D<T>(
			X < other.X ? X : other.X,
			Y < other.Y ? Y : other.Y
  		);
	}

	Vector2D<T> max(const Vector2D &other) const
	{
		return Vector2D<T>(
			X > other.X ? X : other.X,
			Y > other.Y ? Y : other.Y
  		);
	}*/

	unsigned long long getHash() const
	{
		return (unsigned long long)X << (sizeof(X) * 8) | (unsigned long long)Y;
	}

	T X, Y;
};
