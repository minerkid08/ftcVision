#include "opencv2/core.hpp"
#include "opencv2/core/matx.hpp"
#include "opencv2/core/types.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <opencv2/opencv.hpp>
#include <sstream>

#define linesPerElem 6

// #define IMG
// #define CROP_VIEW

struct Color
{
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

struct ColorRange
{
	Color low;
	Color high;
	char hasLow2;
	Color low2;
	Color high2;
};

cv::Vec3b Color_vec(const Color* c)
{
	return {c->r, c->g, c->b};
}

ColorRange redRange = {
	.low = {160, 150, 20},
	.high = {180, 255, 255},
	.hasLow2 = 1,
	.low2 = {0, 150, 20},
	.high2 = {20, 255, 255},
};

ColorRange blueRange = {
	.low = {0, 50, 50},
	.high = {80, 255, 255},
	.hasLow2 = 0,
};

ColorRange blueRange2 = {
	.low = {128, 20, 20},
	.high = {198, 255, 255},
	.hasLow2 = 0,
};

int i2 = 0;
float focalLength = 0.0f;

cv::Mat process(const cv::Mat& inMat)
{
	cv::Mat mat;
	ColorRange* range = &blueRange;
	cv::cvtColor(inMat, mat, cv::COLOR_RGB2HSV);

	int width = mat.size().width;
	int height = mat.size().height;

#ifdef CROP_VIEW

	cv::Rect r;
	r.x = 0;
	r.y = 0;
	r.width = width / 2;
	r.height = height / 2;

	width = width / 2;
	height = height / 2;

	cv::Mat m = (*mat)(r);
#else
	cv::Mat m = mat;
#endif

	cv::Mat m2;

	cv::inRange(m, Color_vec(&range->low), Color_vec(&range->high), m2);
	if (range->hasLow2)
	{
		cv::Mat m3;
		cv::inRange(m, Color_vec(&range->low2), Color_vec(&range->high2), m3);
		cv::add(m3, m2, m2);
	}

	std::vector<std::vector<cv::Point>> countors;

	cv::findContours(m2, countors, cv::RetrievalModes::RETR_LIST, cv::ContourApproximationModes::CHAIN_APPROX_NONE);

	if (countors.size() == 0)
		return mat;

	int elemCount = 0;
	cv::cvtColor(m, m, cv::COLOR_HSV2RGB);
	cv::line(m, {0, height / 2}, {width, height / 2}, cv::Vec3b(0, 255, 0));
	cv::line(m, {width / 2, 0}, {width / 2, height}, cv::Vec3b(0, 255, 0));
	for (const std::vector<cv::Point>& countor : countors)
	{
		cv::RotatedRect rect = cv::minAreaRect(countor);

		if (rect.size.height < 40)
			continue;
		if (rect.size.width < 40)
			continue;

		float rectWidth;
		float rectHeight;
		if (rect.size.width > rect.size.height)
		{
			rectWidth = rect.size.height;
			rectHeight = rect.size.width;
		}
		else
		{
			rectWidth = rect.size.width;
			rectHeight = rect.size.height;
		}

		elemCount++;
		cv::Point2f points[4];
		rect.points(points);
		cv::Point2f aabb[4];

		cv::circle(m, rect.center, 10, {0, 255, 0});

		float minX = rect.center.x;
		float maxX = rect.center.x;
		float minY = rect.center.y;
		float maxY = rect.center.y;

		for (const cv::Point& point : countor)
		{
			minX = __min(point.x, minX);
			maxX = __max(point.x, maxX);
			minY = __min(point.y, minY);
			maxY = __max(point.y, maxY);
		}

		aabb[0].x = minX;
		aabb[0].y = minY;
		aabb[1].x = minX;
		aabb[1].y = maxY;
		aabb[2].x = maxX;
		aabb[2].y = maxY;
		aabb[3].x = maxX;
		aabb[3].y = minY;

		float aabbX = (minX + maxX) / 2;
		float aabbY = (minY + maxY) / 2;
		float aabbW = maxX - minX;
		float aabbH = maxY - minY;

		printf("element %d\n", elemCount);
		printf("rotated rect: x: %.2f, y: %.2f, w: %.2f, h: %.2f, r: %.2f\n", rect.center.x, rect.center.y,
			   rectWidth, rectHeight, rect.angle);
		printf("aabb: x: %.2f, y: %.2f, w: %.2f, h: %.2f\n", aabbX, aabbY, aabbW, aabbH);
		printf("xoff: %.2f, yoff: %.2f\n", rect.center.x - (width / 2.0f), rect.center.y - (height / 2.0f));
		float distx = (1.5f * focalLength) / rectWidth;
		float inPerPx = 1.5f / rectWidth;
		float disty = inPerPx * (rect.center.x - (width / 2.0f));
		printf("x dist: %.2f\n", distx);
		printf("y dist: %.2f\n", disty);

		for (int i = 0; i < 4; i++)
		{
			cv::line(m, points[i], points[i == 3 ? 0 : i + 1], cv::Vec3b(255, 0, 0));
			cv::line(m, aabb[i], aabb[i == 3 ? 0 : i + 1], cv::Vec3b(0, 0, 255));
		}
	}
	if (elemCount < i2)
	{
		int c = i2 - elemCount;
		for (int i = 0; i < c * linesPerElem; i++)
		{
			printf("\033[2K\033[1B");
		}
		printf("\033[%dA", c * linesPerElem);
	}
	if (elemCount > 0)
		printf("\033[%dA", elemCount * linesPerElem);
	i2 = elemCount;
	return m;
}

void calibrate()
{
	printf("calibrating\n");

	std::vector<float> distances;
	{
		std::ifstream distFile("calib/calibDistances.txt");
		std::stringstream sstream;
		sstream << distFile.rdbuf();
		char buf[12];
		int ind = 0;
		std::string s = sstream.str();
		for (char c : s)
		{
			if (c == '\r')
				continue;
			if (c == '\n')
			{
				buf[ind] = 0;
				float n = std::atof(buf);
				distances.push_back(n);
				ind = 0;
				continue;
			}
			buf[ind++] = c;
		}
		buf[ind] = 0;
		float n = std::atof(buf);
		distances.push_back(n);
	}
	printf("calib distances loaded, %d numbers\n", distances.size());

	std::vector<float> focalLengths;

	int i = 0;
	for (std::filesystem::path p : std::filesystem::directory_iterator("calib"))
	{
		if (p.extension() != ".jpg")
			continue;
		cv::Mat mat = cv::imread(p.string().c_str(), cv::IMREAD_COLOR);
		if (mat.data == nullptr)
		{
			printf("error: unable to read calib image. \"%s\"\n", p.string().c_str());
			fflush(stdout);
			exit(1);
		}
		ColorRange* range = &blueRange;
		cv::cvtColor(mat, mat, cv::COLOR_RGB2HSV);

		cv::Mat m = mat;

		cv::Mat m2;

		cv::inRange(m, Color_vec(&range->low), Color_vec(&range->high), m2);
		if (range->hasLow2)
		{
			cv::Mat m3;
			cv::inRange(m, Color_vec(&range->low2), Color_vec(&range->high2), m3);
			cv::add(m3, m2, m2);
		}

		std::vector<std::vector<cv::Point>> countors;

		cv::findContours(m2, countors, cv::RetrievalModes::RETR_LIST, cv::ContourApproximationModes::CHAIN_APPROX_NONE);

		if (countors.size() == 0)
		{
			printf("error: no elements found in calib image\n");
			fflush(stdout);
			exit(1);
		}

		bool bk = false;
		for (const std::vector<cv::Point>& countor : countors)
		{
			cv::RotatedRect rect = cv::minAreaRect(countor);

			if (rect.size.height < 40)
				continue;
			if (rect.size.width < 40)
				continue;

			float rectWidth;
			float rectHeight;
			if (rect.size.width > rect.size.height)
			{
				rectWidth = rect.size.height;
				rectHeight = rect.size.width;
			}
			else
			{
				rectWidth = rect.size.width;
				rectHeight = rect.size.height;
			}

			float dist = distances[i++];
			float width = 1.5f;

			focalLengths.push_back((rectWidth * dist) / width);

			bk = true;
			break;
		}
		if (bk)
			continue;
		printf("error: no valid elements found in calib image\n");
		fflush(stdout);
		exit(1);
	}
	printf("processed %d images\n", i);
	float sum = 0;
	for (float f : focalLengths)
		sum += f;
	focalLength = sum / focalLengths.size();
	printf("focal length %.2f\n", focalLength);
	printf("done\n");
}

int main(int argc, const char** argv)
{
	printf("\033[2J\033[H\033[8m");
	calibrate();
#ifdef IMG
	cv::Mat mat = cv::imread("image3.png", cv::IMREAD_COLOR);
	printf("start width %i, height %i\n", mat.size().width, mat.size().height);
	process(&mat);
#else
	cv::Mat mat;

	cv::VideoCapture c(1, cv::CAP_DSHOW);

	if (!c.isOpened())
	{
		printf("error: cant open camera\n");
		fflush(stdout);
		return 1;
	}

	printf("opened webcam\n");

	c.read(mat);
#endif
	printf("width %i, height %i\n", mat.size().width, mat.size().height);

	printf("----recognition data----\n");

	cv::Mat mat2;

	while (true)
	{
#ifndef IMG
		if (!c.read(mat))
			break;
		mat2 = process(mat);
#endif

		cv::imshow("window", mat2);
		if (cv::waitKey(1) == 'q')
			break;
	}

	printf("\033[%dB", linesPerElem * i2);
	printf("----end recognition data----\n");
	if (!cv::imwrite("out.jpg", mat2))
	{
		printf("error: failed to write mat to file \"out.jpg\"\n");
		fflush(stdout);
		return 1;
	}

	if (!cv::imwrite("outNoOverlay.jpg", mat))
	{
		printf("error: failed to write mat to file \"outNoOverlay.jpg\"\n");
		fflush(stdout);
		return 1;
	}

	return 0;
}
