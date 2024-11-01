#include <opencv2/opencv.hpp>

void process(cv::Mat& mat);

int main(int argc, const char** argv)
{
	cv::Mat mat = cv::imread("image2.jpg", cv::IMREAD_COLOR);

	int len = mat.cols * 3;
	unsigned char* ptr = mat.ptr<unsigned char>(200);
	std::string out;
	for (int i = 0; i < len; i++)
	{
		if (ptr[i] > 128)
		{
			out += 'F';
		}
		else
		{
			out += '0';
		}
	}
	std::cout << out;
	return 0;

	process(mat);

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
};

struct Color
{
	unsigned char b;
	unsigned char g;
	unsigned char r;
};

inline bool checkPixel(const cv::Mat& mat, Vec2 pos)
{
	const Color& elem = *mat.ptr<Color>(pos.y, pos.x);
	if (elem.r > 0 || elem.g > 0 || elem.b > 0)
	{
		std::cout << cv::format("%i, %i, %i: %i, %i\n", elem.r, elem.g, elem.b, pos.x, pos.y);
	}
	return elem.r > 128;
}

void process(cv::Mat& mat)
{
	std::cout << sizeof(Color) << '\n';
	cv::Mat m;

	cv::inRange(mat, cv::Vec3b(0, 0, 128), cv::Vec3b(128, 128, 255), m);

	Vec2 rectPos = {-1, -1};


	for (int x = 0; x < m.cols; x++)
	{
		bool bk = false;
		for (int y = 0; y < m.rows; y++)
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

	Color& elem = mat.ptr<Color>(rectPos.y)[rectPos.x];
	Color& elem2 = m.ptr<Color>(rectPos.y)[rectPos.x];

	std::cout << cv::format("%i, %i, %i\n", elem.r, elem.g, elem.b);
	std::cout << cv::format("%i, %i, %i\n", elem2.r, elem2.g, elem2.b);

	// elem.r = 255;
	// elem.g = 255;
	// elem.b = 255;

	bool tracing = true;
	Vec2 curPos = rectPos;
	while (tracing)
	{
		if (checkPixel(m, curPos + Vec2(1, 0)))
		{
			curPos = curPos + Vec2(1, 0);
			continue;
		}

		if (checkPixel(m, curPos + Vec2(1, 1)))
		{
			curPos = curPos + Vec2(1, 1);
			continue;
		}

		if (checkPixel(m, curPos + Vec2(0, 1)))
		{
			curPos = curPos + Vec2(0, 1);
			continue;
		}

		if (checkPixel(m, curPos + Vec2(-1, 1)))
		{
			curPos = curPos + Vec2(-1, 1);
			continue;
		}
		break;
	}

	std::cout << rectPos.x << ", " << rectPos.y << '\n';
	std::cout << curPos.x << ", " << curPos.y;

	m.copyTo(mat);

	cv::line(mat, cv::Point(curPos.x, curPos.y), cv::Point(rectPos.x, rectPos.y), cv::Vec3b(255, 255, 255));
}
