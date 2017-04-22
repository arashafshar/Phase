#include <math.h>

#include "Polygon.h"
#include "Exception.h"

Polygon::Polygon()
{
    area_ = 0.;
    centroid_ = Point2D(0., 0.);
}

Polygon::Polygon(const std::vector<Point2D> &vertices)
        :
        Polygon(vertices.begin(), vertices.end())
{

}

Polygon::Polygon(const boost::geometry::model::polygon<Point2D, false, true> &boostPgn)
        :
        poly_(boostPgn)
{
    init();
}

//- Tests
bool Polygon::isInside(const Point2D &testPoint) const
{
    return boost::geometry::within(testPoint, poly_);
}

bool Polygon::isOnEdge(const Point2D &testPoint) const
{
    return boost::geometry::covered_by(testPoint, poly_) && !isInside(testPoint);
}

bool Polygon::isCovered(const Point2D &point) const
{
    return boost::geometry::covered_by(point, poly_);
}

bool Polygon::isBoundedBy(const Point2D &point, Scalar toler) const
{
    return isCovered(point);
}

bool Polygon::isValid() const
{
    return boost::geometry::is_valid(poly_);
}

bool Polygon::isEmpty() const
{
    return vertices().size() == 0;
}

//- Intersections
std::vector<Point2D> Polygon::intersections(const Line2D &line) const
{
    std::vector<Point2D> intersections;

    for (const LineSegment2D &edge: edges())
    {
        Vector2D r2 = edge.ptB() - edge.ptA();
        Vector2D dr0 = edge.ptA() - line.r0();

        Scalar t2 = cross(line.d(), dr0) / cross(line.d(), -r2);

        if (edge.isBounded(edge.ptA() + t2 * r2))
            intersections.push_back(edge.ptA() + t2 * r2);
    }

    return intersections;
}

std::vector<Point2D> Polygon::intersections(const LineSegment2D &line) const
{
    Vector2D r1 = line.ptB() - line.ptA();
    std::vector<Point2D> intersections;

    for (const LineSegment2D &edge: edges())
    {
        Vector2D r2 = edge.ptB() - edge.ptA();
        Vector2D dr0 = edge.ptA() - line.ptA();

        Scalar t1 = cross(dr0, -r2) / cross(r1, -r2);
        Scalar t2 = cross(r1, dr0) / cross(r1, -r2);

        if (line.isBounded(line.ptA() + t1 * r1) && edge.isBounded(edge.ptA() + t2 * r2))
            intersections.push_back(line.ptA() + t1 * r1);
    }

    return intersections;
}

Point2D Polygon::nearestIntersect(const Point2D &point) const
{
    Point2D xc;
    Scalar minDistSqr = std::numeric_limits<Scalar>::infinity();

    for (const LineSegment2D &edge: edges())
    {
        if (!edge.isBounded(point))
            continue;

        Vector2D t = edge.ptB() - edge.ptA();
        t = dot(point - edge.ptA(), t) * t / t.magSqr();

        Vector2D n = point - edge.ptA() - t;

        if (n.magSqr() < minDistSqr)
        {
            minDistSqr = n.magSqr();
            xc = edge.ptA() + t;
        }
    }

    for (const Point2D &vertex: vertices())
        if ((point - vertex).magSqr() < minDistSqr)
        {
            minDistSqr = (point - vertex).magSqr();
            xc = vertex;
        }

    return xc;
}

LineSegment2D Polygon::nearestEdge(const Point2D &point) const
{
    LineSegment2D nearestEdge;
    Scalar minDistSqr = std::numeric_limits<Scalar>::infinity();

    for (const LineSegment2D &edge: edges())
    {
        if (!edge.isBounded(point))
            continue;

        Vector2D t = edge.ptB() - edge.ptA();
        t = dot(point - edge.ptA(), t) * t / t.magSqr();

        Vector2D n = point - edge.ptA() - t;

        if (n.magSqr() < minDistSqr)
        {
            nearestEdge = edge;
            minDistSqr = n.magSqr();
        }
    }

    return nearestEdge;
}

bool Polygon::intersects(const Shape2D &shape) const
{
    return boost::geometry::intersects(poly_, shape.polygonize().boostPolygon());
}

//- Transformations
void Polygon::scale(Scalar factor)
{
    for (Point2D &vtx: boost::geometry::exterior_ring(poly_))
        vtx = factor * (vtx - centroid_) + centroid_;

    init();
}

void Polygon::rotate(Scalar theta)
{
    for (Point2D &vtx: boost::geometry::exterior_ring(poly_))
        vtx = (vtx - centroid_).rotate(theta) + centroid_;
}

Polygon Polygon::scale(Scalar factor) const
{
    std::vector<Point2D> verts;
    verts.reserve(vertices().size());

    for (const Point2D &vtx: boost::geometry::exterior_ring(poly_))
        verts.push_back(factor * (vtx - centroid_) + centroid_);

    return Polygon(verts);
}

//- Translations
Polygon &Polygon::operator+=(const Vector2D &translationVec)
{
    for (Point2D &vtx: boost::geometry::exterior_ring(poly_))
        vtx += translationVec;

    centroid_ += translationVec;

    return *this;
}

Polygon &Polygon::operator-=(const Vector2D &translationVec)
{
    return operator+=(-translationVec);
}

//- Bounding box
boost::geometry::model::box<Point2D> Polygon::boundingBox() const
{
    boost::geometry::model::box<Point2D> box;
    boost::geometry::envelope(boostPolygon(), box);

    return box;
}

std::vector<LineSegment2D> Polygon::edges() const
{
    std::vector<LineSegment2D> edges;

    auto vtxA = vertices().begin();
    auto vtxB = vtxA + 1;

    for (; vtxB != vertices().end(); ++vtxA, ++vtxB)
        edges.push_back(LineSegment2D(*vtxA, *vtxB));

    return edges;
}

//- Protected methods

void Polygon::init()
{
    if (boost::geometry::exterior_ring(poly_).size() > 0)
    {
        boost::geometry::correct(poly_);
        area_ = boost::geometry::area(poly_);
        boost::geometry::centroid(poly_, centroid_);
    }
    else
    {
        area_ = 0.;
        centroid_ = Point2D(0., 0.);
    }
}

//- External functions
Polygon intersectionPolygon(const Polygon &pgnA, const Polygon &pgnB)
{
    std::vector<boost::geometry::model::polygon<Point2D, false, true> > pgn;

    boost::geometry::intersection(pgnA.boostPolygon(), pgnB.boostPolygon(), pgn);

    if (pgn.size() == 0)
        return Polygon();
    else if (pgn.size() > 1)
        throw Exception("Polygon", "intersectionPolygon", "there are two polygons!");

    return Polygon(pgn.front());
}

Polygon difference(const Polygon &pgnA, const Polygon &pgnB)
{
    std::vector<boost::geometry::model::polygon<Point2D, false, true> > pgn;
    boost::geometry::difference(pgnA.boostPolygon(), pgnB.boostPolygon(), pgn);

    if (pgn.size() == 0)
        return Polygon();
    else if (pgn.size() > 1)
        throw Exception("Polygon", "difference", "there is more than one output polygon!");

    return Polygon(pgn.front());
}

Polygon clipPolygon(const Polygon &pgn, const Line2D &line)
{
    std::vector<Point2D> verts;

    for (auto vertIt = pgn.vertices().begin(); vertIt != pgn.vertices().end() - 1; ++vertIt)
    {
        const Vector2D &vtx = *vertIt;
        const Vector2D &nextVtx = *(vertIt + 1);
        Line2D edgeLine = Line2D(vtx, (nextVtx - vtx).normalVec());

        if (line.isApproximatelyOnLine(vtx))
        {
            verts.push_back(vtx); // special case
            continue;
        }
        else if (line.isBelowLine(vtx))
            verts.push_back(vtx);

        std::pair<Point2D, bool> xc = Line2D::intersection(line, edgeLine);

        if (xc.second) // the lines are not paralell, ie xc is valid
        {
            Scalar l = (nextVtx - vtx).magSqr();
            Scalar x = dot(nextVtx - vtx, xc.first - vtx);

            if (x < l && x > 0 && !(xc.first == vtx || xc.first == nextVtx)) // intersection is on the segment
                verts.push_back(xc.first);
        }
    }

    return Polygon(verts);
}
