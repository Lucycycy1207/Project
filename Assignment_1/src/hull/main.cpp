////////////////////////////////////////////////////////////////////////////////
#include <algorithm>
#include <complex>
#include <fstream>
#include <iostream>
#include <numeric>
#include <vector>
////////////////////////////////////////////////////////////////////////////////
#include <stack>

//1. Find the lower - left point in the input point cloud P0.
//2. Sort all input points counter - clockwise with respect to P0.
//3. Greedily compute the convex - hull polygon by iterating over the sorted points.Whenever a "right-turn" is encountered, pop the middle - point from the hull.


typedef std::complex<double> Point;//stores an ordered pair of objects both of type double
typedef std::vector<Point> Polygon;


bool check_equal(Point& p1, Point& p2, Point& p0) {

	Point p3(p1.real() - p0.real(), p1.imag() - p0.imag());
	Point p4(p2.real() - p0.real(), p2.imag() - p0.imag());

	if (arg(p3) == arg(p4)) {
		//same degree
		return true;
	}
	else {
		return false;
	}
}

Point nextToTop(std::stack<Point>& s) {
	Point p = s.top();
	s.pop();
	Point n = s.top();
	s.push(p);
	return n;
}

double inline det(const Point &u, const Point &v) {
	// TODO
	return 0;
}

struct Compare {//fix bug; fixed
	//Complete the comparison structure 
	//to sort points in counter-clockwise order

	Point p0; // Leftmost point of the poly
	bool operator ()(const Point& p1, const Point& p2) {
		// TODO
		Point p3(p1.real() - p0.real(), p1.imag() - p0.imag());
		Point p4(p2.real() - p0.real(), p2.imag() - p0.imag());
		if (arg(p3) < arg(p4)) {//p1 goes first
			return true;
		}
		else if (arg(p3) == arg(p4)) {
			//same degree
			return abs(p3) < abs(p4);//nearest to p0 goes first
		}
		else {
			return false;
		}
	}
};

int orientation(Point a, Point b, Point c) {
	double value = (b.imag() - a.imag()) * (c.real() - b.real())
		-
		(b.real() - a.real()) * (c.imag() - b.imag());
	if (value > 0) {
		//clockwise,not salientangle
		return 2;
	}
	else if (value < 0) {
		//counterclockwise, salientangle
		return 1;
	}
	else {
		//collinear
		return 3;
	}
}
bool inline salientAngle(Point a, Point b, Point c) {//0 - 180 degree
	//b is the middle point here
	// TODO
	//ab = |a||b|cos(theta)
	//transfter to vector A and B
	
	/*
	double AB = A.real() * B.real() + A.imag() * B.imag();
	double abs_AB = abs(A) * abs(B);
	double angle = acos(AB / abs_AB); // in radians
	//no pow() function library included
	//std::cout << "angle: " << acos(0) << "\n";
	double pi = 2 * acos(0.0);
	std::cout << "angle: " << angle << "\n";
	//std::cout << "range: " << pi/2 << "\n";
	if (angle >= 0 && angle <= pi/2) {
		return true;
	}
	else {
		return false;
	}
	*/
	if (orientation(a, b, c) == 1) {
		return true;
	}
	else {
		return false;
	}
	
}

////////////////////////////////////////////////////////////////////////////////

Polygon convex_hull(std::vector<Point> &points) {//in progress
	Compare order;
	// TODO
	//
	//find lower-left most point: min x, min y
	double min_y;//current min y value(imag)
	Point min_p;
	for (Point p : points) {
		if (p == points.front()) {//the first point,
			min_y = p.imag();
			min_p = p;
			
		}
		else if (p.imag() < min_y) {//new low point occur
			min_y = p.imag();
			min_p = p;
			
		}
		else if (p.imag() == min_y) {//same lowest, check smaller x
			if (p.real() < min_p.real()) {//new point has lower x
				min_y = p.imag();
				min_p = p;
			}
		}
	}
	//std::cout << "leftmost point: " << min_p << "\n";
	//leftmost point: (242.034,400.447)


	order.p0 = min_p;
	std::sort(points.begin(), points.end(), order);

	size_t p_s = points.size();
	/*
	//check if two or more points have same angle,
	//remove all points instead of the farthest
	//points is sorted
	
	size_t modified = 1;
	for (int i = 1; i<n; i++) {
		//removing i if i and i+1 have same angle
		while (i < n-1 &&
			check_equal(points[i], points[i + 1], order.p0)
			)
		{
			i++;
		}
		points[modified++] = points[i];
	}
	//no same angle points found ,still 3000
	*/
	
	

	Polygon hull;

	// TODO
	
	Point a, b, c;
	if (points.size() < 3) {
		std::cout << "input points less than three points" << "\n";
		return hull;
	} 	

	std::stack<Point> s;
	s.push(points.at(0));
	s.push(points.at(1));
	s.push(points.at(2));
	
	// use salientAngle(a, b, c) here
	//salientAngle(a, b, c);
	Point next;
	for (int i = 3; i < p_s; i++) {
		
		

		//check nexttotop, top, point i
		while ((s.size()>1) && (!salientAngle(nextToTop(s), s.top(), points.at(i)))) {
			s.pop();
			//std::cout << "hfgjfku " << "\n";
		}
		s.push(points.at(i));

	}
	
	size_t total_p = 0;
	while (!s.empty()) {
		//std::cout << s.top() << "\n";
		hull.push_back(s.top());
		s.pop();
		total_p++;
	}

	//std::cout << "point in hull: " << hull.size() << "\n";
	return hull;
}



////////////////////////////////////////////////////////////////////////////////

std::vector<Point> load_xyz(const std::string &filename) {//good so far
	std::vector<Point> points;
	std::ifstream in(filename); //stream class to read from file
	
	//ofstream– This class represents an output stream.It’s used for creating filesand writing information to files.
	//ifstream– This class represents an input stream.It’s used for reading information from data files.
	//fstream– This class generally represents a file stream.It comes with ofstream / ifstream capabilities.This means it’s capable of creating files, writing to files, reading from data files.

	// TODO
	if (!in.is_open()) {

		throw std::runtime_error("failed to open file " + filename);
	
	}
	else {

		std::string line;
		std::string delimiter = " ";
		std::string token;
		size_t pos_start, pos_end;
		double num;
		size_t line_num = 0;
		size_t total_point;
		std::getline(in, line);
		total_point = std::stoi(line);
		//std::cout << "total_point" << total_point << "\n";
		double p1;
		double p2;
		size_t pos;

		while (std::getline(in, line)) {

			pos_start = 0;

			//print out each line
			//std::cout << line << "\n";

			pos = 0;
			while ((pos_end = line.find(delimiter, pos_start)) != std::string::npos) {
				token = line.substr(pos_start, pos_end - pos_start);
				
				//transfer token to number,
				num = std::stod(token);
				//std::cout << "num" << num << "\n";
				
				//int to point type(double)
				if (pos == 0) {//x value
					p1 = num;
					pos = 1;
					pos_start = pos_end + delimiter.length();
				}
				else {//y value
					p2 = num;
					pos_start = pos_end + delimiter.length();
					break;
				}

			}
			//point p = (x,y)
			Point p(p1, p2);
			//put point in points vector
			points.push_back(p);

		}

		//print out all the points
		/*
		for (Point p : points){
			std::cout << p << "\n";
		}
		*/	
		
	}

	return points;
}

void save_obj(const std::string &filename, Polygon &poly) {
	std::ofstream out(filename);
	if (!out.is_open()) {
		throw std::runtime_error("failed to open file " + filename);
	}
	out << std::fixed;
	for (const auto &v : poly) {
		out << "v " << v.real() << ' ' << v.imag() << " 0\n";
	}
	for (size_t i = 0; i < poly.size(); ++i) {
		out << "l " << i+1 << ' ' << 1+(i+1)%poly.size() << "\n";
	}
	out << std::endl;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char * argv[]) {
	if (argc <= 2) {
		std::cerr << "Usage: " << argv[0] << " points.xyz output.obj" << std::endl;
	}
	std::vector<Point> points = load_xyz(argv[1]);
	Polygon hull = convex_hull(points);
	save_obj(argv[2], hull);
	
	return 0;
}
