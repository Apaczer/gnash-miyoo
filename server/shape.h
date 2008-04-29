//   Copyright (C) 2005, 2006, 2007, 2008 Free Software Foundation, Inc.
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//


#ifndef GNASH_SHAPE_H
#define GNASH_SHAPE_H

#include "dsodefs.h"
#include "styles.h"
#include "rect.h"
#include "log.h"

#include <vector> // for path composition
#include <boost/bind.hpp>  
#include <algorithm>


// Forward declarations
namespace gnash {
  class rect; // for path::expandBounds
}

namespace gnash {
  using namespace geometry;

  /// \brief
  /// Together with the previous anchor,
  /// defines a quadratic curve segment.
  template <typename T>
  class Edge
  {
  public:
    Edge()
    {
    }

    Edge(T cx, T cy, T ax, T ay)
      :
      cp(cx, cy),
      ap(ax, ay)
    {
    }

    Edge(const Edge<T>& from)
      : cp(from.cp),
        ap(from.ap)
    {
    }

    template <typename U>
    Edge(const Edge<U>& from)
      : cp(from.cp.x, from.cp.y),
        ap(from.ap.x, from.ap.y)
    {
    }

    Edge(const Point2d<T>& ncp, const Point2d<T>& nap)
      :
      cp(ncp),
      ap(nap)
    {
    }

    template <typename U>
    Edge(const Point2d<U>& ncp, const Point2d<U>& nap)
      :
      cp(ncp.x, ncp.y),
      ap(nap.y, nap.y)
    {
    }

    bool isStraight() const
    {
      return cp == ap;
    }

    bool is_straight() const { return isStraight(); }
    
    /// Transform the edge according to the given matrix.
    void
    transform(const matrix& mat)
    {
      mat.transform(ap);
      mat.transform(cp);
    }

    /// Return squared distance between point pt and segment A-B
    template <typename U, typename V, typename W>
    static float
    squareDistancePtSeg(const Point2d<U>& p, const Point2d<V>& A,
            const Point2d<W>& B)
    {
      float dx = B.x - A.x;
      float dy = B.y - A.y;

            /* if start==end, then use pt distance */
            if ( dx == 0 && dy == 0 ) {
        return p.squareDistance(A);
      }

      float pdx = p.x - A.x;
      float pdy = p.y - A.y;

            float u = (pdx * dx + pdy * dy) / float(dx*dx + dy*dy);

            if (u<0)
      {
        //cout << "R was < 0 " << endl;
        return p.squareDistance(A); 
      }

            if (u>1)
      {
        //cout << "R was > 1 " << endl;
        return p.squareDistance(B);
      }

      Point2d<float> px;
      px.x = A.x + u * (B.x - A.x);
      px.y = A.y + u * (B.y - A.y);

      //cout << "R was between 0 and 1, u is " << u << " px : " << px.x << "," << px.y << endl;
  
      return p.squareDistance(px);
    }

    /// Return distance between point pt and segment A-B
    template <typename U>
    static float
    distancePtSeg(const Point2d<U>& pt, const Point2d<U>& A, const Point2d<U>& B)
    {
      float square = squareDistancePtSeg(pt, A, B);
      return sqrt(square);
    }

    /// Find point of the quadratic curve defined by points A,C,B
    //
    /// @param A The first point
    /// @param C The second point (control point)
    /// @param B The third point (anchor point)
    /// @param ret The point to write result into
    /// @param t the step factor between 0 and 1
    ///

    template <typename U>
    static geometry::Point2d<float>
    pointOnCurve(const Point2d<U>& A,
           const Point2d<U>& C,
           const Point2d<U>& B, float t)
    {
      if ( t == 0.0 ) return Point2d<float>(A.x, A.y);
      else if ( t == 1.0 ) return Point2d<float>(B.x, B.y);

      Point2d<float> Q1(A, C, t);
      Point2d<float> Q2(C, B, t);
       Point2d<float> R(Q1, Q2, t);

#ifdef DEBUG_POINT_ON_CURVE
      std::stringstream ss;
      ss <<  "A:" << A << " C:" << C << " B:" << B
        << " T:" << t
        << " Q1:" << Q1 << " Q2:" << Q2
        << " R:" << R;
      log_debug("%s", ss.str().c_str());
#endif

      return R;
    }

    /// Return square distance between point pt and the point on curve found by
    /// applying the T parameter to the quadratic bezier curve function
    //
    /// @param A The first point of the bezier curve
    /// @param C The second point of the bezier curve (control point)
    /// @param B The third point of the bezier curve (anchor point)
    /// @param p The point we want to compute distance from 
    /// @param t the step factor between 0 and 1
    ///
    template <typename U>
    static float squareDistancePtCurve(const Point2d<U>& A,
               const Point2d<U>& C,
               const Point2d<U>& B,
               const Point2d<U>& p, float t)
    {
      return p.squareDistance( pointOnCurve(A, C, B, t) );
    }
    
  //private:
    // *quadratic* bezier: point = p0 * t^2 + p1 * 2t(1-t) + p2 * (1-t)^2
    Point2d<T> cp; // "control" point
    Point2d<T> ap; // "anchor" point
  };

  typedef Edge<int> edge;


  /// \brief
  /// A subset of a shape -- a series of edges sharing a single set
  /// of styles. The template argument defines the data type to be used for
  /// coordinates; this can be either a fixed or floating point type.
  template <typename T>
  class DSOEXPORT Path
  {
  public:

    /// Default constructor
    //
    /// @param newShape
    ///  True if this path starts a new subshape
    ///
    Path(bool newShape = false)
      : m_new_shape(newShape)
    {
      reset(0, 0, 0, 0, 0);
    }

    template <typename U>
    Path(const Path<T>& from)
      : m_fill0(from.m_fill0),
        m_fill1(from.m_fill1),
        m_line(from.m_line),
        ap(from.ap),
        m_edges(from.m_edges),
        m_new_shape(from.m_new_shape)        
    {
    }

    template <typename U>
    Path(const Path<U>& from)
      : m_fill0(from.m_fill0),
        m_fill1(from.m_fill1),
        m_line(from.m_line),
        ap(from.ap.x, from.ap.y),
        m_edges(from.m_edges.begin(), from.m_edges.end()),
        m_new_shape(from.m_new_shape)        
    {
    }

    /// Initialize a path 
    //
    /// @param ax
    ///  X coordinate of path origin in TWIPS
    ///
    /// @param ay
    ///  Y coordinate in path origin in TWIPS
    ///
    /// @param fill0
    ///  Fill style index for left fill (1-based).
    ///  Zero means NO style.
    ///
    /// @param fill1
    ///  Fill style index for right fill (1-based)
    ///  Zero means NO style.
    ///
    /// @param line
    ///  Line style index for right fill (1-based).
    ///  Zero means NO style.
    ///
    /// @param newShape
    ///  True if this path starts a new subshape
    Path(T ax, T ay, int fill0, int fill1, int line, bool newShape)
      :
      m_new_shape(newShape)
    {
      reset(ax, ay, fill0, fill1, line);
    }

    /// Re-initialize a path, maintaining the "new shape" flag untouched
    //
    /// @param ax
    ///  X coordinate of path origin in TWIPS
    ///
    /// @param ay
    ///  Y coordinate in path origin in TWIPS
    ///
    /// @param fill0
    ///  Fill style index for left fill
    ///
    /// @param fill1
    ///  Fill style index for right fill
    //
    /// @param line
    ///  Line style index for right fill
    ///
    void  reset(T ax, T ay, int fill0, int fill1, int line)
        // Reset all our members to the given values, and clear our edge list.
    {
      ap.x = ax;
      ap.y = ay;
      m_fill0 = fill0;
      m_fill1 = fill1;
      m_line = line;

      m_edges.resize(0);
      assert(is_empty());
    }

    /// Return true if we have no edges.
    bool  
    is_empty() const

    {
        return m_edges.empty();
    }

    /// Expand given rect to include bounds of this path
    //
    /// @param r
    ///  The rectangle to expand with our own bounds
    ///
    /// @param thickness
    ///  The thickess of our lines, half the thickness will
    ///  be added in all directions in swf8+, all of it will
    ///  in swf7-
    ///
    /// @param swfVersion
    ///  SWF version to use.
    ///
    void
    expandBounds(rect& r, unsigned int thickness, int swfVersion) const
    {
      const Path<T>&  p = *this;

      size_t nedges = m_edges.size();
      if ( ! nedges ) return; // this path adds nothing

      if (thickness)
      {
        // NOTE: Half of thickness would be enough (and correct) for
        // radius, but that would not match how Flash calculates the
        // bounds using the drawing API.                        
        unsigned int radius = swfVersion < 8 ? thickness : thickness/2.0;

        r.expand_to_circle(ap.x, ap.y, radius);
        for (unsigned int j = 0; j<nedges; j++)
        {
          r.expand_to_circle(m_edges[j].ap.x, m_edges[j].ap.y, radius);
          r.expand_to_circle(m_edges[j].cp.x, m_edges[j].cp.y, radius);
        }

        return;
      }

      r.expand_to_point(ap.x, ap.y);
      for (unsigned int j = 0; j<nedges; j++)
      {
        r.expand_to_point(m_edges[j].ap.x, p.m_edges[j].ap.y);
                    r.expand_to_point(m_edges[j].cp.x, p.m_edges[j].cp.y);
      }
    }

    /// @{ Primitives for the Drawing API
    ///
    /// Name of these functions track Ming interface
    ///

    /// Draw a straight line.
    //
    /// Point coordinates are relative to path origin
    /// and expressed in TWIPS.
    ///
    /// @param x
    ///  X coordinate in TWIPS
    ///
    /// @param y
    ///  Y coordinate in TWIPS
    ///
    void 
    drawLineTo(T dx, T dy)
    {
      m_edges.push_back(Edge<T>(dx, dy, dx, dy)); 
    }

    /// Draw a curve.
    //
    /// Offset values are relative to path origin and
    /// expressed in TWIPS.
    ///
    /// @param cx
    ///  Control point's X coordinate.
    ///
    /// @param cy
    ///  Control point's Y coordinate.
    ///
    /// @param ax
    ///  Anchor point's X ordinate.
    ///
    /// @param ay
    ///  Anchor point's Y ordinate.
    ///
    void 
    drawCurveTo(T cdx, T cdy, T adx, T ady)
    {
      m_edges.push_back(Edge<T>(cdx, cdy, adx, ady)); 
    }

    /// Remove all edges and reset style infomation 
    void clear()
    {
      m_edges.resize(0);
      m_fill0 = m_fill1 = m_line = 0;
    }

    /// @} Primitives for the Drawing API



    /// Close this path with a straight line, if not already closed
    void
    close()
    {
      // nothing to do if path there are no edges
      if ( m_edges.empty() ) return;

      // Close it with a straight edge if needed
      const Edge<T>& lastedge = m_edges.back();
      if ( lastedge.ap != ap )
      {
        Edge<T> newedge(ap, ap);
        m_edges.push_back(newedge);
      }
    }


    /// \brief
    /// Return true if the given point is withing the given squared distance
    /// from this path edges.
    //
    /// NOTE: if the path is empty, false is returned.
    ///
    bool
    withinSquareDistance(const Point2d<float>& p, float dist) const
    {
//#define GNASH_DEBUG_PATH_DISTANCE 1

      size_t nedges = m_edges.size();

      if ( ! nedges ) return false;

      int_point px(ap);
      for (size_t i=0; i<nedges; ++i)
      {
        const Edge<T>& e = m_edges[i];
        int_point np(e.ap);

        if ( e.isStraight() )
        {
          float d = Edge<float>::squareDistancePtSeg(p, px, np);
#ifdef GNASH_DEBUG_PATH_DISTANCES
	  log_debug("squaredDistance %s-%s to %s == %g (asked for %g)", px, np, p, d, dist);
#endif
          if ( d < dist ) return true;
        }
        else
        {
#ifdef GNASH_DEBUG_PATH_DISTANCES
	  log_debug("edge %d is a curve", i);
#endif // DEBUG_MOUSE_ENTITY_FINDING
          // It's a curve !

          const int_point& A = px;
          const int_point& C = e.cp;
          const int_point& B = e.ap;

          // TODO: early break if point is NOT in the area
          //       defined by the triangle ACB and it's square 
          //       distance from it is > then the requested one

          // Approximate the curve to segCount segments
          // and compute distance of query point from each
          // segment.
          //
          // TODO: find an apprpriate value for segCount based
          //       on rendering scale ?
          //
          int segCount = 10; 
          float_point p0(A.x, A.y);
          for (int i=1; i<=segCount; ++i)
          {
            float t1 = (float)i/segCount;
            float_point p1 = Edge<T>::pointOnCurve(A, C, B, t1);

            // distance from point and segment being an approximation
            // of the curve 
            float d = Edge<T>::squareDistancePtSeg(p, p0, p1);

#ifdef GNASH_DEBUG_PATH_DISTANCES
            log_debug("squaredDistance (curve)%s-%s-%s to %s == %g (asked for %g)", A, C, B, p, d, dist);
#endif // DEBUG_MOUSE_ENTITY_FINDING

            //float d = edge::squareDistancePtCurve(A, C, B, p, t);
            //log_debug("Factor %26.26g, distance %g (asked %g)", t, sqrt(d), sqrt(dist));
            if ( d <= dist ) return true;

            p0.setTo(p1.x, p1.y);
          }
        }

        px = np;
      }

      return false;
    }



    /// Transform all path coordinates according to the given matrix.
    void
    transform(const matrix& mat)
    {
      using namespace boost;

      mat.transform(ap);
      std::for_each(m_edges.begin(), m_edges.end(),
                    bind(&Edge<T>::transform, _1, ref(mat)));                
    }    

    /// Set this path as the start of a new (sub)shape
    void setNewShape() { m_new_shape=true; }

    /// Return true if this path starts a new (sub)shape
    bool getNewShape() const { return m_new_shape; }

    /// Return true if this path contains no edges
    bool  empty() const
    {
      return is_empty();
    }

    /// Set the fill to use on the left side
    //
    /// @param f
    ///  The fill index (1-based).
    ///  When this path is added to a shape_character_def,
    ///  the index (decremented by 1) will reference an element
    ///  in the fill_style vector defined for that shape.
    ///  If zero, no fill will be active.
    ///
    void setLeftFill(unsigned f)
    {
      m_fill0 = f;
    }

    unsigned getLeftFill() const
    {
      return m_fill0;
    }

    /// Set the fill to use on the left side
    //
    /// @param f
    ///  The fill index (1-based).
    ///  When this path is added to a shape_character_def,
    ///  the index (decremented by 1) will reference an element
    ///  in the fill_style vector defined for that shape.
    ///  If zero, no fill will be active.
    ///
    void setRightFill(unsigned f)
    {
      m_fill1 = f;
    }

    unsigned getRightFill() const
    {
      return m_fill1;
    }

    /// Set the line style to use for this path
    //
    /// @param f
    ///  The line_style index (1-based).
    ///  When this path is added to a shape_character_def,
    ///  the index (decremented by 1) will reference an element
    ///  in the line_style vector defined for that shape.
    ///  If zero, no fill will be active.
    ///
    void setLineStyle(unsigned i)
    {
      m_line = i;
    }

    unsigned getLineStyle() const
    {
      return m_line;
    }

    /// Return the number of edges in this path
    size_t size() const
    {
      return m_edges.size();
    }

    /// Return a reference to the Nth edge 
    Edge<T>& operator[] (size_t n)
    {
      return m_edges[n];
    }

    /// Return a const reference to the Nth edge 
    const Edge<T>& operator[] (size_t n) const
    {
      return m_edges[n];
    }

    /// Returns true if this path begins a new subshape. <-- VERIFYME
    bool isNewShape() const
    {
      return m_new_shape;
    }
    

  //private:

    /// Left fill style index (1-based)
    unsigned m_fill0;

    /// Right fill style index (1-based)
    unsigned m_fill1;

    /// Line style index (1-based)
    unsigned m_line;

    /// Path/shape origin 
    Point2d<T> ap; 

    /// Edges forming the path
    std::vector< Edge<T> > m_edges;

    /// This flag is set when the path is the first one of a new "sub-shape".
    /// All paths with a higher index in the list belong to the same 
    /// shape unless they have m_new_shape==true on their own.
    /// Sub-shapes affect the order in which outlines and shapes are rendered.
    bool m_new_shape;
  };
  
  typedef Path<int> path;

}  // end namespace gnash


#endif // GNASH_SHAPE_H


// Local Variables:
// mode: C++
// c-basic-offset: 8 
// tab-width: 8
// indent-tabs-mode: t
// End:
