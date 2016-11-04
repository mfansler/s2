// Copyright 2005 Google Inc. All Rights Reserved.

#ifndef UTIL_GEOMETRY_S2CELLUNION_H_
#define UTIL_GEOMETRY_S2CELLUNION_H_

#include <vector>
using std::vector;

#include "base/integral_types.h"
#include "base/logging.h"
#include "base/macros.h"
#include "s2region.h"
#include "s2cellid.h"

class S1Angle;
class S2Cell;

// An S2CellUnion is a region consisting of cells of various sizes.  Typically
// a cell union is used to approximate some other shape.  There is a tradeoff
// between the accuracy of the approximation and how many cells are used.
// Unlike polygons, cells have a fixed hierarchical structure.  This makes
// them more suitable for optimizations based on preprocessing.
class S2CellUnion : public S2Region {
 public:
  // The default constructor does nothing.  The cell union cannot be used
  // until one of the Init() methods is called.
  S2CellUnion() {}

  // Populates a cell union with the given S2CellIds or 64-bit cells ids, and
  // then calls Normalize().  The InitSwap() version takes ownership of the
  // vector data without copying and clears the given vector.  These methods
  // may be called multiple times.
  void Init(vector<S2CellId> const& cell_ids);
  void Init(vector<uint64> const& cell_ids);
  void InitSwap(vector<S2CellId>* cell_ids);

  // Like Init(), but does not call Normalize().  The cell union *must* be
  // normalized before doing any calculations with it, so it is the caller's
  // responsibility to make sure that the input is normalized.  This method is
  // useful when converting cell unions to another representation and back.
  // These methods may be called multiple times.
  void InitRaw(vector<S2CellId> const& cell_ids);
  void InitRaw(vector<uint64> const& cell_ids);
  void InitRawSwap(vector<S2CellId>* cell_ids);

  // Gives ownership of the vector data to the client without copying, and
  // clears the content of the cell union.  The original data in cell_ids
  // is lost if there was any.  This is the opposite of InitRawSwap().
  void Detach(vector<S2CellId>* cell_ids);

  // Convenience methods for accessing the individual cell ids.
  int num_cells() const { return cell_ids_.size(); }
  S2CellId const& cell_id(int i) const { return cell_ids_[i]; }

  // Direct access to the underlying vector for STL algorithms.
  vector<S2CellId> const& cell_ids() const { return cell_ids_; }

  // Normalizes the cell union by discarding cells that are contained by other
  // cells, replacing groups of 4 child cells by their parent cell whenever
  // possible, and sorting all the cell ids in increasing order.  Returns true
  // if the number of cells was reduced.
  //
  // This method *must* be called before doing any calculations on the cell
  // union, such as Intersects() or Contains().
  bool Normalize();

  // Replaces "output" with an expanded version of the cell union where any
  // cells whose level is less than "min_level" or where (level - min_level)
  // is not a multiple of "level_mod" are replaced by their children, until
  // either both of these conditions are satisfied or the maximum level is
  // reached.
  //
  // This method allows a covering generated by S2RegionCoverer using
  // min_level() or level_mod() constraints to be stored as a normalized cell
  // union (which allows various geometric computations to be done) and then
  // converted back to the original list of cell ids that satisfies the
  // desired constraints.
  void Denormalize(int min_level, int level_mod,
                   vector<S2CellId>* output) const;

  // If there are more than "excess" elements of the cell_ids() vector that
  // are allocated but unused, reallocate the array to eliminate the excess
  // space.  This reduces memory usage when many cell unions need to be held
  // in memory at once.
  void Pack(int excess = 0);

  // Return true if the cell union contains the given cell id.  Containment is
  // defined with respect to regions, e.g. a cell contains its 4 children.
  // This is a fast operation (logarithmic in the size of the cell union).
  bool Contains(S2CellId const& id) const;

  // Return true if the cell union intersects the given cell id.
  // This is a fast operation (logarithmic in the size of the cell union).
  bool Intersects(S2CellId const& id) const;

  // Return true if this cell union contain/intersects the given other cell
  // union.
  bool Contains(S2CellUnion const* y) const;
  bool Intersects(S2CellUnion const* y) const;

  // Initialize this cell union to the union, intersection, or
  // difference (x - y) of the two given cell unions.
  // Requires: x != this and y != this.
  void GetUnion(S2CellUnion const* x, S2CellUnion const* y);
  void GetIntersection(S2CellUnion const* x, S2CellUnion const* y);
  void GetDifference(S2CellUnion const* x, S2CellUnion const* y);

  // Specialized version of GetIntersection() that gets the intersection of a
  // cell union with the given cell id.  This can be useful for "splitting" a
  // cell union into chunks.
  void GetIntersection(S2CellUnion const* x, S2CellId const& id);

  // Expands the cell union by adding a "rim" of cells on expand_level
  // around the union boundary.
  //
  // For each cell c in the union, we add all cells at level
  // expand_level that abut c.  There are typically eight of those
  // (four edge-abutting and four sharing a vertex).  However, if c is
  // finer than expand_level, we add all cells abutting
  // c.parent(expand_level) as well as c.parent(expand_level) itself,
  // as an expand_level cell rarely abuts a smaller cell.
  //
  // Note that the size of the output is exponential in
  // "expand_level".  For example, if expand_level == 20 and the input
  // has a cell at level 10, there will be on the order of 4000
  // adjacent cells in the output.  For most applications the
  // Expand(min_radius, max_level_diff) method below is easier to use.
  void Expand(int expand_level);

  // Expand the cell union such that it contains all points whose distance to
  // the cell union is at most "min_radius", but do not use cells that are
  // more than "max_level_diff" levels higher than the largest cell in the
  // input.  The second parameter controls the tradeoff between accuracy and
  // output size when a large region is being expanded by a small amount
  // (e.g. expanding Canada by 1km).  For example, if max_level_diff == 4 the
  // region will always be expanded by approximately 1/16 the width of its
  // largest cell.  Note that in the worst case, the number of cells in the
  // output can be up to 4 * (1 + 2 ** max_level_diff) times larger than the
  // number of cells in the input.
  void Expand(S1Angle const& min_radius, int max_level_diff);

  // Create a cell union that corresponds to a continuous range of cell ids.
  // The output is a normalized collection of cell ids that covers the leaf
  // cells between "min_id" and "max_id" inclusive.
  // Requires: min_id.is_leaf(), max_id.is_leaf(), min_id <= max_id.
  void InitFromRange(S2CellId const& min_id, S2CellId const& max_id);

  // The number of leaf cells covered by the union.
  // This will be no more than 6*2^60 for the whole sphere.
  uint64 LeafCellsCovered() const;

  // Approximate this cell union's area by summing the average area of
  // each contained cell's average area, using the AverageArea method
  // from the S2Cell class.
  // This is equivalent to the number of leaves covered, multiplied by
  // the average area of a leaf.
  // Note that AverageArea does not take into account distortion of cell, and
  // thus may be off by up to a factor of 1.7.
  // NOTE: Since this is proportional to LeafCellsCovered(), it is
  // always better to use the other function if all you care about is
  // the relative average area between objects.
  double AverageBasedArea() const;

  // Calculates this cell union's area by summing the approximate area for each
  // contained cell, using the ApproxArea method from the S2Cell class.
  double ApproxArea() const;

  // Calculates this cell union's area by summing the exact area for each
  // contained cell, using the Exact method from the S2Cell class.
  double ExactArea() const;

  ////////////////////////////////////////////////////////////////////////
  // S2Region interface (see s2region.h for details):

  virtual S2CellUnion* Clone() const;
  virtual S2Cap GetCapBound() const;
  virtual S2LatLngRect GetRectBound() const;

  // This is a fast operation (logarithmic in the size of the cell union).
  virtual bool Contains(S2Cell const& cell) const;

  // This is a fast operation (logarithmic in the size of the cell union).
  virtual bool MayIntersect(S2Cell const& cell) const;

  virtual bool VirtualContainsPoint(S2Point const& p) const {
    return Contains(p);  // The same as Contains() below, just virtual.
  }

  virtual void Encode(Encoder* const encoder) const {
    LOG(FATAL) << "Unimplemented";
  }
  virtual bool Decode(Decoder* const decoder) { return false; }

  // The point 'p' does not need to be normalized.
  // This is a fast operation (logarithmic in the size of the cell union).
  bool Contains(S2Point const& p) const;

 private:
  vector<S2CellId> cell_ids_;

  DISALLOW_EVIL_CONSTRUCTORS(S2CellUnion);
};

// Return true if two cell unions are identical.
bool operator==(S2CellUnion const& x, S2CellUnion const& y);

#endif  // UTIL_GEOMETRY_S2CELLUNION_H_
