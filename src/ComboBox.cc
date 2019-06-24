
# include "ComboBox.hh"
# include "LinearLayout.hh"
# include "LabelWidget.hh"
# include "PictureWidget.hh"
# include <core_utils/CoreException.hh>

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

      m_state(State::Closed),
      m_closedBox(),

      m_activeItem(-1),
      m_items()
    {
      // Assign the z order for this widget: it should be drawn in front of other
      // regular widgets.
      setZOrder(1);

      // Build the layout for this component.
      build();
    }

    ComboBox::~ComboBox() {}

    void
    ComboBox::insertItem(const int index,
                         const std::string& text,
                         const std::string& icon)
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

    bool
    ComboBox::mouseButtonReleaseEvent(const core::engine::MouseEvent& e) {
      // Clicking on a combobox can have different effects based on its
      // current state.
      // Basically the comboxbox has two macro states: it can be dropped
      // or closed. When the combobox is dropped, all the options in the
      // limit of `m_maxVisibleItems` are displayed. In the closed state,
      // only the currently selected option is displayed.
      // When the macro state is closed, a click on the combobox will
      // make it drop and display all the options. When the macro state
      // is dropped, a click on the combobox will correspond to a click
      // on an option and thus update the active item. It will also close
      // the combobox as an option has been selected.
      // Of course we first need to determine whether the click has been
      // triggered inside the widget before doing anything.

      // Check whether the click is inside the widget.
      bool inside = isInsideWidget(e.getMousePosition());

      // If the click was not in the widget, close the combobox.
      if (!inside) {
        setState(State::Closed);
      }

      // If the click occurred inside the widget, we need to determine
      // the current state of ths combobox. If the combobox is closed
      // the click will trigger a drop event. If the combobox is already
      // dropped we need to handle the selection of a widget.

      if (inside && isClosed()) {
        // Create a drop event.
        setState(State::Dropped);
      }

      // At this point we clicked on the combobox while it is dropped: we
      // need to determine whether the click occurred on the base item (i.e.
      // the one currently selected) or if a new item was selected.
      // This is not handled in this method though, usually we should get
      // a signal from the corresponding element in the `listItemClicked`
      // slot.
      // So we don't have anything more to do here.

      // Use base handler to determine whether the event was recognized.
      return core::SdlWidget::mouseButtonReleaseEvent(e);
    }

    bool
    ComboBox::resizeEvent(const core::engine::ResizeEvent& e) {
      // We need to assign this box as the closed box.
      m_closedBox = e.getNewSize();

      // TODO: Handle case where the combobox is opened.

      // USe the base method to provide the return value.
      return SdlWidget::resizeEvent(e);
    }

    void
    ComboBox::build() {
      // Assign a linear layout which will allow positionning items and icons.
      LinearLayoutShPtr layout = std::make_shared<LinearLayout>(
        "combobox_layout",
        this,
        LinearLayout::Direction::Horizontal,
        0.0f,
        1.0f
      );

      // Create two children: a picture widget and a label widget which will be used
      // to represent the items of this combobox.
      PictureWidget* icon = new PictureWidget(
        std::string("combobox_icon"),
        std::string(),
        PictureWidget::Mode::Fit,
        this
      );

      LabelWidget* text = new LabelWidget(
        std::string("combobox_text"),
        std::string(),
        std::string("data/fonts/times.ttf"),
        15,
        LabelWidget::HorizontalAlignment::Left,
        LabelWidget::VerticalAlignment::Center,
        this,
        core::engine::Color::NamedColor::Silver
      );

      // Add these items to the layout.
      layout->addItem(icon);
      layout->addItem(text);

      // And assign the layout to this widget.
      setLayout(layout);
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

    void
    ComboBox::setActiveItem(const int& index) {
      // Remove the corresponding item if it exists.
      if (index < 0 || index >= getItemsCount()) {
        error(
          std::string("Cannot set active item ") + std::to_string(index) + " in combobox",
          std::string("No such item")
        );
      }

      // If the index corresponds to the currently active item we don't have to
      // do anything.
      if (index == m_activeItem) {
        return;
      }

      // Assign the active item.
      m_activeItem = index;

      // We need to activate the corresponding text and icon from the internal
      // table.
      getChildAs<PictureWidget>(std::string("combobox_icon"))->setImagePath(m_items[m_activeItem].icon);
      getChildAs<LabelWidget>(std::string("combobox_text"))->setText(m_items[m_activeItem].text);
    }

    void
    ComboBox::setState(const State& state) {
      // First check that the state is actually different from the current state
      if (m_state == state) {
        // Nothing to do.
        return;
      }

      // We need to assign a new size to the combobox based on the desired state.
      // The current closed size is always saved in the `m_closedBox` and in order
      // to obtain the opened size we multiply it by the number of item visible
      // in the combobox.

      // Assume the size is for the closed size.
      utils::Boxf newSize = m_closedBox;
      if (state == State::Dropped) {
        newSize = getDroppedSize();
      }

      log("Old size is " + m_closedBox.toString() + ", new is " + newSize.toString());

      // Update the internal state.
      m_state = state;

      // Now proceed to posting a resize event with the new size.
      postEvent(std::make_shared<core::engine::ResizeEvent>(
        newSize,
        LayoutItem::getRenderingArea()
      ));

      // We also need to either create the needed icon and text widgets
      // or make them visible if they already exist.
      const int count = getVisibleItemsCount();
      for (int id = 0 ; id < count ; ++id) {
        // Try to set the icon and the visible status for the picture widget.
        PictureWidget* icon = getChildOrNull<PictureWidget>(getIconNameFromID(id));

        if (icon == nullptr) {
          // Create the item.
          // TODO: We need to insert it into a layout of some sort, probably a grid layout global to this widget.
          icon = new PictureWidget(
            getIconNameFromID(id),
            std::string(),
            PictureWidget::Mode::Fit,
            this
          );
        }

        // Now we either created the icon widget if needed or found it if it
        // already existed. We can assign properties to the picture widget.
        icon->setImagePath(m_items[m_activeItem].icon);
        icon->setVisible(true);

        // Try to set the text and the visible status for the label widget.
        LabelWidget* text = getChildOrNull<LabelWidget>(getTextNameFromID(id));

        if (text == nullptr) {
          // Create the item.
          text = new LabelWidget(
            getTextNameFromID(id),
            m_items[m_activeItem].text,
            std::string("data/fonts/times.ttf"),
            15,
            LabelWidget::HorizontalAlignment::Left,
            LabelWidget::VerticalAlignment::Center,
            this,
            core::engine::Color::NamedColor::Yellow
          );
        }

        // Now we either created the label widget if needed or found it if it
        // already existed. We can assign properties to the label widget.
        text->setText(m_items[m_activeItem].text);
        text->setVisible(true);
      }
    }

    utils::Boxf
    ComboBox::getDroppedSize() const noexcept {
      // We basically scale the closed size by the number of items to
      // display. We use either the minimum number of items available
      // or the count of items.
      // We at least want to keep one element even though no elements
      // are registered in the combobox.
      const int scaling = getVisibleItemsCount();

      // Compute the delta.
      const float delta = (scaling - 1) * m_closedBox.h();

      return utils::Boxf(
        m_closedBox.x(),
        m_closedBox.y() - delta / 2.0f,
        m_closedBox.w(),
        m_closedBox.h() * scaling
      );
    }

  }
}