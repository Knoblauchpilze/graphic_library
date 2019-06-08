#ifndef    LINEARLAYOUT_HXX
# define   LINEARLAYOUT_HXX

# include "LinearLayout.hh"

namespace sdl {
  namespace graphic {

    inline
    int
    LinearLayout::addItem(core::LayoutItem* item) {
      // Use the specialized handler and assume that we add this item
      // at the end of the layout.
      return addItem(item, getItemsCount());
    }

    inline
    int
    LinearLayout::addItem(core::LayoutItem* item,
                          const int& index)
    {
      // We want to insert the `item` at logical position `index`. This
      // includes registering the item in the layout as usual but we also
      // need to register it into the internal `m_idsToPosition` table.
      //
      // Also we should update position of existing items so that we still
      // have consistent ids ranging from `0` all the way to `getItemsCount()`.

      // The first thing is to add the item using the base handler: this will
      // provide us a first index to work with.
      int id = Layout::addItem(item);

      // Check whether the insertion was successful.
      if (id < 0) {
        return id;
      }

      // At this point we know that the item could successfully be added to
      // the layout. We still need to account for its logical position
      // described by the input `index`.
      // We have three main cases:
      // 1. `index < 0` in which case we insert the `item` before the first
      //    element of the layout.
      // 2. `index >= size` in which case we insert the `item` after the
      //    last element of the layout.
      // 3. We insert the element in the middle of the layout.
      //
      // In all 3 cases we need to relabel the items which come after the
      // newly inserted item so that we keep some kind of consistency.

      // First, normalize the index: don't forget that the current size of
      // the layout *includes* the item we want to insert (because `addItem`
      // has already been called).
      int normalized = std::min(std::max(0, index), getItemsCount() - 1);

      // Update the label of existing items if it exceeds the new desired
      // logical index.
      for (unsigned id = 0u ; id < m_idsToPosition.size() ; ++id) {
        if (m_idsToPosition[id] >= normalized) {
          ++m_idsToPosition[id];
        }
      }

      // Now we have a valid set of labels with a hole at the position the
      // new `item` should be inserted: let's fix that.
      m_idsToPosition.insert(m_idsToPosition.cbegin() + normalized, id);

      // Return the value provided by the base handler.
      return id;
    }

    inline
    int
    LinearLayout::removeItem(core::LayoutItem* item) {
      // TODO: Maybe reimplement `removeItemFromIndex` instead ?
      // While removing the item, we need to make sure that the internal table
      // indicating the logical position of the item stays consistent.
      // To do so, we need to perform an update of the internal table based on
      // the new real position of the items.
      // Basically let's consider the following situation:
      //
      // m_idsToPosition[0] = 1
      // m_idsToPosition[1] = 0
      // m_idsToPosition[2] = 2
      // m_idsToPosition[3] = 4
      // m_idsToPosition[4] = 3
      //
      // We want to remove the item `2`. Note that this will not correspond to
      // the real item at position `2` but rather to the item at *logical* id
      // `2`.
      // If we simply remove the item from the internal table we will get the
      // following array:
      //
      // m_idsToPosition[0] = 1
      // m_idsToPosition[1] = 0
      // m_idsToPosition[3] = 4
      // m_idsToPosition[4] = 3
      //
      // It is not hard to see that we have a problem: the real index `3` or `4`
      // do not match any widget anymore and the next time we will try to assign
      // positions to these, we will face trouble.
      // The solution is to rely on the fact that the `Layout` class will perform
      // a collapse of the remaining items with a position larger than the one we
      // want to erase. So we could just decrease by one the real index of all the
      // items which had an id greater than the index of the item we just removed.
      // Doing this we would end up with the following:
      //
      // m_idsToPosition[0] = 1
      // m_idsToPosition[1] = 0
      // m_idsToPosition[2] = 3
      // m_idsToPosition[3] = 2
      //
      // Which is now correct.
      // And we also have to remove the corresponding logical entry in the internal
      // table before considering ourselves done.

      // So first perform the deletion of the required item.
      int rmID = Layout::removeItem(item);

      // Handle cases where the deletion failed.
      if (rmID < 0) {
        return rmID;
      }

      // Now update the real indices of the widgets with a real index greater than
      // the one we removed.
      // In the meantime we can try tofind the logical id of the item which has just
      // been removed.
      int logicalIDOfRm = -1;

      for (unsigned id = 0 ; id < m_idsToPosition.size() ; ++id) {
        if (m_idsToPosition[id] > rmID) {
          --m_idsToPosition[id];
        }
        if (m_idsToPosition[id] == rmID) {
          logicalIDOfRm = id;
        }
      }

      // Check whether we could find this item.
      if (logicalIDOfRm < 0) {
        log(
          std::string("Could not update logical ids in linear layout after removing item \"") + item->getName() + "\"",
          utils::Level::Warning
        );
        return rmID;
      }

      // Now erase the corresponding entry in the internal table.
      m_idsToPosition.erase(m_idsToPosition.cbegin() + logicalIDOfRm);

      // Return the value provided by the base handler.
      return rmID;
    }

    inline
    const core::Layout::Direction&
    LinearLayout::getDirection() const noexcept {
      return m_direction;
    }

    inline
    const float&
    LinearLayout::getComponentMargin() const noexcept {
      return m_componentMargin;
    }

    inline
    utils::Sizef
    LinearLayout::computeAvailableSize(const utils::Boxf& totalArea) const noexcept {
      // We assume the layout is not empty.

      // Use the base class method to provide a first rough estimation.
      utils::Sizef internalSize = Layout::computeAvailableSize(totalArea);

      // Handle horizontal layout.
      if (getDirection() == Direction::Horizontal) {
        return internalSize - utils::Sizef((getItemsCount() - 1.0f) * m_componentMargin, 0.0f);
      }

      // Assume the layout is vertical.
      return internalSize - utils::Sizef(0.0f, (getItemsCount() - 1.0f) * m_componentMargin);
    }

    inline
    utils::Sizef
    LinearLayout::computeDefaultItemBox(const utils::Sizef& area,
                                        const unsigned& itemsCount) const {
      // We assume the layout is not empty.
      if (getDirection() == Direction::Horizontal) {
        return utils::Sizef(
          allocateFairly(area.w(), itemsCount),
          area.h()
        );
      }
      else if (getDirection() == Direction::Vertical) {
        return utils::Sizef(
          area.w(),
          allocateFairly(area.h(), itemsCount)
        );
      }
      error(std::string("Unknown direction when updating layout (direction: ") + std::to_string(static_cast<int>(getDirection())) + ")");

      // Make compiler quiet about reaching end of non void function.
      return utils::Sizef();
    }

  }
}

#endif    /* LINEARLAYOUT_HXX */
