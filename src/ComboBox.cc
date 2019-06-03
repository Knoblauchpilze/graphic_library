
# include "ComboBox.hh"

namespace sdl {
  namespace graphic {

    ComboBox::ComboBox(const std::string& name,
                       const InsertPolicy& policy,
                       core::SdlWidget* parent,
                       const utils::Sizef& area,
                       const int maxVisibleItems):
      core::SdlWidget(name, area, parent, core::engine::Color::NamedColor::White),

      m_insertPolicy(policy),
      m_maxVisibleItems(maxVisibleItems),

      m_activeItem(-1),
      m_items()
    {}

    ComboBox::~ComboBox() {}

    void
    ComboBox::insertItem(const int index,
                         const std::string& text,
                         PictureWidget* icon)
    {
      // Create a new item from the input data.
      ComboBoxItem item = ComboBoxItem{
        text,
        icon
      };

      // Check the position of insertion: we want to correctly handle
      // cases where the index is either negative or larger than the
      // current size of the internal `m_items` vector.
      if (index < 0) {
        m_items.insert(m_items.cbegin(), item);
      }
      else if (index >= getItemsCount()) {
        m_items.push_back(item);
      }
      else {
        m_items.insert(m_items.cbegin() + index, item);
      }

      // We also need to update the active item if any: indeed if the
      // active item was after the item which has just been inserted
      // we need to increase the value of the `m_activeItem` by one
      // to make it for the newly inserted item.
      if (hasActiveItem() && m_activeItem >= index) {
        ++m_activeItem;
      }

      // Check whether we need to modify the display for this combobox. This
      // can be the case if:
      // 1. there is no an item in the combo box.
      // 2. there was only one item in the combo box and it was selected.
      if (getItemsCount() == 1 || (getItemsCount() == 2 && hasActiveItem())) {
        makeContentDirty();
      }
    }

    void
    ComboBox::removeItem(const int index) {
      // Remove the corresponding item if it exists.
      if (index < 0 || index >= getItemsCount()) {
        error(
          std::string("Cannot remove item ") + std::to_string(index) + " from combobox",
          std::string("No such item")
        );
      }

      // We need to delete the icon associated to this item if any.
      if (m_items[index].icon != nullptr) {
        delete m_items[index].icon;
      }

      // Perform the deletion.
      m_items.erase(m_items.cbegin() + index);

      // Check whether this was the active item.
      if (m_activeItem == index) {
        // We need to make the next item active if it is still possible.
        // The only way it can fail is if the active item was the last one
        // on the list.
        if (m_activeItem >= getItemsCount()) {
          m_activeItem = getItemsCount() - 1;
        }
      }

      // We need to update the content.
      makeContentDirty();
    }

    void
    ComboBox::drawContentPrivate(const utils::Uuid& /*uuid*/) const {
      // TODO: Should probably be implemented as a linear layout with two items: the
      // icon and the text.
      // We could assign a max size on the widget which would be based on the dimensions
      // of the largest item. The icon also has a maximum size, which is hard coded and
      // consistent with the specifications of the parameters of the `addItem` method which
      // should be completed with missing information. Note that the labels are updated and
      // not recreated each time: we do not have cache for each combobox item.
      // We should maybe replace the picture widget used by each item by a single string
      // representing the path to the item.
      // This would allow for example to use a selector widget instead of a picture widget
      // so that we could have some cache mechanism for pictures.
    }

    std::pair<int, bool>
    ComboBox::getIndexFromInsertPolicy(const std::string& text) const {
      // Compute the alphabetical position of the input text based on the
      // existing items already registered in the combobox.
      // We could retrieve all the keys, add the input `text` then perform
      // a sort and obtain the id. But we can also just scan all the
      // existing keys and compare each one to the input `text` and work
      // out the position of the text from there.
      int alphabeticalRank = getItemsCount();
      for (unsigned id = 0u ; id < m_items.size() ; ++id) {
        if (m_items[id].text.compare(text) < 0) {
          // The new text should be inserted at least before this item.
          alphabeticalRank = id - 1;
        }
      }

      // We only need to take care of the case where the input `text` should
      // be inserted before the first registered item. We will end up with an
      // `alphabeticalRank` of `-1` which we should transform into `0`. This
      // is not a problem because the `AlphabeticalOrder` does not erase any
      // existing item.
      alphabeticalRank = std::max(alphabeticalRank, 0);


      // Disitnguish according to the insertion policy.
      switch (m_insertPolicy) {
        case InsertPolicy::InsertAtTop:
          return std::make_pair(0, false);
        case InsertPolicy::InsertAtCurrent:
          return std::make_pair(getActiveItem(), true);
        case InsertPolicy::InsertAtBottom:
          return std::make_pair(getItemsCount(), false);
        case InsertPolicy::InsertAfterCurrent:
          return std::make_pair(getActiveItem() + 1, false);
        case InsertPolicy::InsertBeforeCurrent:
          return std::make_pair(getActiveItem(), false);
        case InsertPolicy::InsertAlphabetically:
          return std::make_pair(alphabeticalRank, false);
        case InsertPolicy::NoInsert:
        default:
          break;
      }

      // If we reach this point it means that we were not able to determine
      // a valid insertion index from the policy. Raise an error.
      error(
        std::string("Could not determine insertion index from policy ") + getNameFromPolicy(m_insertPolicy),
        std::string("Invalid policy")
      );

      // Silent compiler even though `error` will throw.
      return std::make_pair(0, false);
    }

  }
}