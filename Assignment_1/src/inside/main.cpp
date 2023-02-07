////////////////////////////////////////////////////////////////////////////////
#include <algorithm>
#include <complex>
#include <fstream>
#include <iostream>
#include <numeric>
#include <vector>
////////////////////////////////////////////////////////////////////////////////

typedef std::complex<double> Point;
typedef std::vector<Point> Polygon;

double inline det(const Point &u, const Point &v) {
	// TODO
	//a1*b2-a2*b1
	return (u.real() * v.imag() - u.imag() * v.real());
}

// Return true iff [a,b] intersects [c,d], and store the intersection in ans
bool intersect_segment(const Point &a, const Point &b, const Point &c, const Point &d, Point &ans) {
	// TODO
	//find if two line is parallel or not
	
	Point l1 = b - a;
	Point l2 = d - c;
	if (det(l1,l2) == 0) {//parallel
		std::cout << "is parallel\n";
		return false;
	}
	else {
		//(a1x+b1y+c1 = 0)
		//(a2x+b2y+c2 = 0)
		//x = (b1c2-b2c1)/(a1b2-a2b1)
		//y = (a2c1-a1c2)/(a1b2-a2b1)
		//(y1 – y2)x + (x2 – x1)y + (x1y2 – x2y1) = 0
		//a,b
		double a1 = a.imag() - b.imag();
		double b1 = b.real() - a.real();
		double c1 = a.real() * b.imag() - b.real() * a.imag();
		//c,d
		double a2 = c.imag() - d.imag();
		double b2 = d.real() - c.real();
		double c2 = c.real() * d.imag() - d.real() * c.imag();
		double x = (b1 * c2 - b2 * c1) / (a1 * b2 - a2 * b1);
		double y = (a2 * c1 - a1 * c2) / (a1 * b2 - a2 * b1);
		
		//check if (x, y) on two lines 
		//check on [a,b]
		double max_x = std::max(a.real(), b.real());
		double min_x = std::min(a.real(), b.real());
		double max_y = std::max(a.imag(), b.imag());
		double min_y = std::min(a.imag(), b.imag());
		if (x >= min_x && x<=max_x && y>=min_y && y <= max_y) {
			//it is on [a,b] line segment
			max_x = std::max(c.real(), d.real());
			min_x = std::min(c.real(), d.real());
			max_y = std::max(c.imag(), d.imag());
			min_y = std::min(c.imag(), d.imag());
			

			if (x >= min_x && x<=max_x && y>=min_y && y <= max_y) {
				//it is on [c,d] line segment
				ans = Point(x, y);
				return true;

			}
		}
		return false;

	}
}

////////////////////////////////////////////////////////////////////////////////
bool on_vertex(const Polygon& poly, const Point& p) {
	for (Point q : poly) {
		if (q == p) {
			return true;
		}
	}
	return false;
}
bool is_inside(const Polygon &poly, const Point &query) {//need to updat, count =0or1
	// 1. Compute bounding box and set coordinate of a point outside the polygon
	// TODO
	Point outside(0, 0);
	Point a = poly.at(0);
	Point b;
	size_t count = 0;
	size_t vertex_p = 0;
	Point intersect;
	//std::cout << "check point: " << query << "\n";size_t tmp = 0;

	for (int i = 1; i<poly.size();i++) {
		b = poly.at(i);
		//the a,b here put into intersect_segment,if true, count++

		if (intersect_segment(a, b, query, outside, intersect)) {
			//check if the intersect is the vertex point, if yes, vertex_p++
			if (on_vertex(poly, intersect)) {
				vertex_p++;
			}
			count++;
		}
		a = b;
	}
	//link first and last element
	b = poly.at(0);
	if (intersect_segment(a, b, query, outside, intersect)) {
		if (on_vertex(poly, intersect)) {
			vertex_p++;
		}
		count++;
	}
	//std::cout << "count: " << count << "\n";
	if (vertex_p != 0) {
		count -= vertex_p % 2;
	}
	if ((count % 2 == 0) || count == 0) {
		return false;
	}
	// 2. Cast a ray from the query point to the 'outside' point, count number of intersections
	// TODO

	return true;
}

////////////////////////////////////////////////////////////////////////////////

std::vector<Point> load_xyz(const std::string &filename) {
	std::vector<Point> points;
	std::ifstream in(filename);
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

Polygon load_obj(const std::string& filename) {
	std::ifstream in(filename);
	// TODO
	Polygon poly;
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
		double p1;
		double p2;
		size_t pos;
		size_t end = 0;

		while (std::getline(in, line)) {

			pos_start = 0;
			pos = 0;
			//print out each line
			
			for (int i = 0; i < 3; i++) {
				pos_end = line.find(delimiter, pos_start);//find the element end pos
				token = line.substr(pos_start, pos_end - pos_start);//find the element
		
				
				if (i != 0) {//not v symbol at front
					num = std::stod(token);
					if (pos == 0) {//x value
						p1 = num;
						pos = 1;
						pos_start = pos_end + delimiter.length();
					}
					else if (pos == 1) {//y value
						p2 = num;
					}
				}
				else if (token == "v"){//v skip
					pos_start = pos_end + delimiter.length();
				}
				else {//f value end
					end = 1;
					break;
				}

			}

			pos = 0;
			
			//point p = (x,y)
			if (end != 1) {
				Point p(p1, p2);
				poly.push_back(p);
			}
			else {//at the end
				break;
			}
		
			//put point in points vector
			//points.push_back(p);

		}

		//print out all the points
		/*
		for (Point p : poly){
			std::cout << p << "\n";
		}
		*/
		return poly;
	}
}

void save_xyz(const std::string &filename, const std::vector<Point> &points) {
	// TODO
	//format
	//1st line: number of points N
	//space separate point coordinates
	std::ofstream out(filename);
	if (!out.is_open()) {
		throw std::runtime_error("failed to open file " + filename);
	}
	out << std::fixed;
	for (const auto& point : points) {
		out << point.real() << ' ' << point.imag() << " 0\n";
	}
	out << std::endl;

}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char * argv[]) {
	if (argc <= 3) {
		std::cerr << "Usage: " << argv[0] << " points.xyz poly.obj result.xyz" << std::endl;
	}
	size_t c = 0;
	std::vector<Point> points = load_xyz(argv[1]);
	Polygon poly = load_obj(argv[2]);
	std::vector<Point> result;

	for (size_t i = 0; i < points.size(); ++i) {
		if (is_inside(poly, points[i])) {
			result.push_back(points[i]);
			c++;
		}
	}
	
	//std::cout<<"total count: "<<c<<"\n";
	save_xyz(argv[3], result);
	
	return 0;
}
