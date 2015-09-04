#pragma once

#include <Eigen/Eigen>

#include "LineSegment.h"

namespace SceneGraph
{

class AxisAlignedBoundingBox
{
public:
    inline AxisAlignedBoundingBox()
    {
        Clear();
    }

    inline AxisAlignedBoundingBox(const AxisAlignedBoundingBox& bbox)
        : boxmin(bbox.boxmin), boxmax(bbox.boxmax)
    {
    }

    inline AxisAlignedBoundingBox(const Eigen::Vector3d boxmin, const Eigen::Vector3d boxmax)
        : boxmin(boxmin), boxmax(boxmax)
    {
    }

    inline AxisAlignedBoundingBox(Eigen::Matrix4d& T_ba, const AxisAlignedBoundingBox& bb_a)
    {
        Clear();
        Insert(T_ba, bb_a);
    }

    // Construct bounding box from Frustum.
    inline AxisAlignedBoundingBox(
        const Eigen::Matrix4d& T_wc,
        double w, double h,
        double fu, double fv, double u0, double v0,
        double near, double far
    ) {
        Clear();
        InsertFrustum(T_wc,w,h,fu,fv,u0,v0,near,far);
    }

    inline bool Empty() const {
        const Eigen::Vector3d size = Size();
        return size(0) < 0 && size(1) < 0 && size(2) < 0;
    }

    inline Eigen::Vector3d& Min() {
        return boxmin;
    }

    inline const Eigen::Vector3d& Min() const {
        return boxmin;
    }

    inline Eigen::Vector3d& Max() {
        return boxmax;
    }

    inline const Eigen::Vector3d& Max() const {
        return boxmax;
    }

    inline Eigen::Vector3d Center() const {
        return Min() + Size()/2.0;
    }

    inline void Clear()
    {
        boxmin = Eigen::Vector3d(std::numeric_limits<float>::max(),std::numeric_limits<float>::max(),std::numeric_limits<float>::max());
        boxmax = Eigen::Vector3d(-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max(),-std::numeric_limits<float>::max());
    }
    
    inline void SetZero()
    {
        boxmin = Eigen::Vector3d::Zero();
        boxmax = Eigen::Vector3d::Zero();
    }

    // Expand bounding box to include p
    template<typename Derived> inline
    void Insert( const Eigen::MatrixBase<Derived>& p)
    {
        boxmax = p.cwiseMax(boxmax);
        boxmin = p.cwiseMin(boxmin);
    }

    // Expand bounding box to include p
    inline void Insert(const Eigen::Matrix4d& T_ba, const Eigen::Vector3d p_a)
    {
        const Eigen::Vector3d p_b = T_ba.block<3,3>(0,0) * p_a + T_ba.block<3,1>(0,3);
        boxmax = p_b.cwiseMax(boxmax);
        boxmin = p_b.cwiseMin(boxmin);
    }

    // Expand bounding box to include bb
    inline void Insert(const AxisAlignedBoundingBox& bb)
    {
        boxmax = bb.boxmax.cwiseMax(boxmax);
        boxmin = bb.boxmin.cwiseMin(boxmin);
    }

    // Expand bounding box to include p
    inline void Insert(const Eigen::Matrix4d& T_ba, const AxisAlignedBoundingBox& bb_a)
    {
        if(!bb_a.Empty()) {
            // TODO: This can probably be simplified since we have an expected ordering for min/max
            Insert(T_ba, Eigen::Vector3d(bb_a.boxmin(0), bb_a.boxmin(1), bb_a.boxmin(2)) );
            Insert(T_ba, Eigen::Vector3d(bb_a.boxmin(0), bb_a.boxmin(1), bb_a.boxmax(2)) );
            Insert(T_ba, Eigen::Vector3d(bb_a.boxmin(0), bb_a.boxmax(1), bb_a.boxmin(2)) );
            Insert(T_ba, Eigen::Vector3d(bb_a.boxmin(0), bb_a.boxmax(1), bb_a.boxmax(2)) );
            Insert(T_ba, Eigen::Vector3d(bb_a.boxmax(0), bb_a.boxmin(1), bb_a.boxmin(2)) );
            Insert(T_ba, Eigen::Vector3d(bb_a.boxmax(0), bb_a.boxmin(1), bb_a.boxmax(2)) );
            Insert(T_ba, Eigen::Vector3d(bb_a.boxmax(0), bb_a.boxmax(1), bb_a.boxmin(2)) );
            Insert(T_ba, Eigen::Vector3d(bb_a.boxmax(0), bb_a.boxmax(1), bb_a.boxmax(2)) );
        }
    }

    inline void InsertFrustum(
        const Eigen::Matrix4d& T_wc,
        double w, double h,
        double fu, double fv, double u0, double v0,
        double near, double far
    ) {
        // Insert each edge of frustum into bounding box
        const Eigen::Vector3d c_w = T_wc.block<3,1>(0,3);
        const Eigen::Vector3d ray_tl = T_wc.block<3,3>(0,0) * Eigen::Vector3d((0-u0)/fu,(0-v0)/fv, 1);
        const Eigen::Vector3d ray_tr = T_wc.block<3,3>(0,0) * Eigen::Vector3d((w-u0)/fu,(0-v0)/fv, 1);
        const Eigen::Vector3d ray_bl = T_wc.block<3,3>(0,0) * Eigen::Vector3d((0-u0)/fu,(h-v0)/fv, 1);
        const Eigen::Vector3d ray_br = T_wc.block<3,3>(0,0) * Eigen::Vector3d((w-u0)/fu,(h-v0)/fv, 1);

        Insert(c_w + near*ray_tl);
        Insert(c_w + near*ray_tr);
        Insert(c_w + near*ray_bl);
        Insert(c_w + near*ray_br);
        Insert(c_w + far*ray_tl);
        Insert(c_w + far*ray_tr);
        Insert(c_w + far*ray_bl);
        Insert(c_w + far*ray_br);
    }

    // Contract bounding box to represent intersection (common space)
    // between this and bb
    inline void Intersect(const AxisAlignedBoundingBox& bb)
    {
        boxmin = bb.boxmin.cwiseMax(boxmin);
        boxmax = bb.boxmax.cwiseMin(boxmax);
    }

    inline Eigen::Vector3d Size() const
    {
        return boxmax - boxmin;
    }

    inline double Volume() const
    {
        Eigen::Vector3d s = Size();
        return s(0)*s(1)*s(2);
    }

    // Useful for bullet collision box shape
    inline Eigen::Vector3d HalfSizeFromOrigin() const
    {
        return boxmax.cwiseMax((Eigen::Vector3d)(-1.0*boxmin) );
    }
    
    inline void ScaleFromCenter(const Eigen::Vector3d& scale)
    {
        Eigen::Vector3d center = Center();
        boxmin.array() = scale.array() * (boxmin - center).array() + center.array();
        boxmax.array() = scale.array() * (boxmax - center).array() + center.array();
    }
    
    // Returns lambda ray parameter of intersection, or max val if no intersection
    template <typename T>
    T RayIntersect(const LineSegment<T>& ray) const
    {
        if( !Empty() ) {
            // http://www.cs.utah.edu/~awilliam/box/box.pdf
            const Eigen::Matrix<T,3,1> tminbound = (boxmin.cast<T>() - ray.P()).array() / ray.Dir().array();
            const Eigen::Matrix<T,3,1> tmaxbound = (boxmax.cast<T>() - ray.P()).array() / ray.Dir().array();
            const Eigen::Matrix<T,3,1> tmin = tminbound.cwiseMin(tmaxbound);
            const Eigen::Matrix<T,3,1> tmax = tminbound.cwiseMax(tmaxbound);
            const T max_tmin = tmin.maxCoeff();
            const T min_tmax = tmax.minCoeff();
            if(max_tmin <= min_tmax ) {
                return max_tmin;
            }
        }
        return std::numeric_limits<GLdouble>::max();
    }
    

protected:
    Eigen::Vector3d boxmin;
    Eigen::Vector3d boxmax;
};


inline std::ostream& operator<<(std::ostream& os, const AxisAlignedBoundingBox& bbox)
{
    os << "(" << bbox.Min().transpose() << ")";
    os << " - ";
    os << "(" << bbox.Max().transpose() << ")";
    return os;
}

}
