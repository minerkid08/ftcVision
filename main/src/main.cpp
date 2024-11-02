#include <opencv2/opencv.hpp>

void process(cv::Mat& mat);

int main(int argc, const char** argv)
{
	cv::Mat mat = cv::imread("image2.jpg", cv::IMREAD_COLOR);

	process(mat);

	cv::imwrite("out.jpg", mat);

	cv::imshow("window", mat);
	cv::waitKey(0);
	return 0;
}

struct Vec2
{
	int x;
	int y;

	Vec2(){};
	Vec2(int _x, int _y)
	{
		x = _x;
		y = _y;
	}

	Vec2 operator+(const Vec2& other)
	{
		return {x + other.x, y + other.y};
	}

	bool operator==(const Vec2& other)
	{
		return other.x == x && other.y == y;
	}

	operator const cv::Point() const
	{
		return cv::Point(x, y);
	}

	static Vec2 midpoint(const Vec2& a, const Vec2& b)
	{
		return {(a.x + b.x) / 2, (a.y + b.y) / 2};
	}
};

struct Color
{
	unsigned char b;
	unsigned char g;
	unsigned char r;
};

inline bool checkPixel(const cv::Mat& mat, Vec2 pos, Vec2 offset = {0, 0})
{
	return mat.ptr<unsigned char>(pos.y + offset.y)[pos.x + offset.x];
}

Vec2 traceLine(const cv::Mat& m, Vec2 curPos, const Vec2* offsets, int len, bool strictTracing = true)
{
	char e = (strictTracing ? -1 : -2);
	while (true)
	{
		bool set = false;
		for (int i = 0; i < len; i++)
		{
			if (e == i)
				continue;
			if (checkPixel(m, curPos, offsets[i]))
			{
				if (i == 0 && e == -1)
					e = 2;
				if (i == 2 && e == -1)
					e = 0;
				set = true;
				curPos.x += offsets[i].x;
				curPos.y += offsets[i].y;
				break;
			}
		}
		if (!set)
			break;
	}
	return curPos;
}

void process(cv::Mat& mat)
{
	cv::Mat m;

	cv::inRange(mat, cv::Vec3b(0, 0, 190), cv::Vec3b(170, 170, 255), m);

	/*for (int y = 0; y < m.rows; y++)
	{
		for (int x = 0; x < m.cols; x++)
		{
			unsigned char c = m.ptr<unsigned char>(y)[x];
      if(c)
			mat.ptr<Color>(y)[x] = {c, c, c};
		}
	}*/

	Vec2 rectPos = {-1, -1};

	for (int y = 0; y < m.rows; y++)
	{
		bool bk = false;
		for (int x = 0; x < m.cols; x++)
		{
			if (checkPixel(m, {x, y}))
			{
				rectPos.x = x;
				rectPos.y = y;
				bk = true;
				break;
			}
		}
		if (bk)
			break;
	}

	if (rectPos.x == -1 && rectPos.y == -1)
	{
		std::cout << "cant find rectangle";
		return;
	}

	Vec2 offsets[] = {{-1, 0}, {-1, 1}, {0, 1}};
	Vec2 curPos = traceLine(m, rectPos, offsets, 3, false);

	Vec2 offsets2[] = {{0, 1}, {1, 1}, {1, 0}};
	Vec2 curPos2 = traceLine(m, curPos, offsets2, 3, false);

	// unsigned char rectColor = 128;
	cv::Vec3b rectColor(255, 0, 0);

	cv::line(mat, cv::Point(curPos.x, curPos.y), cv::Point(rectPos.x, rectPos.y), rectColor);
	cv::line(mat, cv::Point(curPos2.x, curPos2.y), cv::Point(curPos.x, curPos.y), rectColor);

	Vec2 midpoint = Vec2::midpoint(rectPos, curPos2);

	Vec2 point4(midpoint.x * 2 - curPos.x, midpoint.y * 2 - curPos.y);
	cv::line(mat, cv::Point(curPos2.x, curPos2.y), cv::Point(point4.x, point4.y), rectColor);
	cv::line(mat, cv::Point(rectPos.x, rectPos.y), cv::Point(point4.x, point4.y), rectColor);

	//std::cout << midpoint.x << ", " << midpoint.y << '\n';

	//std::vector<cv::Point> points = {rectPos, curPos, curPos2, point4};

	//cv::fillPoly(mat, cv::InputArrayOfArrays(points), rectColor);

	cv::circle(mat, cv::Point(midpoint.x, midpoint.y), 5, cv::Vec3b(0, 255, 0));
}
