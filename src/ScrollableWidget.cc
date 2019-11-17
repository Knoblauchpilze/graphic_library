
# include "ScrollableWidget.hh"

namespace sdl {
  namespace graphic {

    ScrollableWidget::ScrollableWidget(const std::string& name,
                                       core::SdlWidget* parent,
                                       const utils::Sizef& area):
      core::SdlWidget(name, area, parent),

      m_supportName(),
      m_coordsToFollow(nullptr)
    {
      // We don't want the widget to be sensitive to hover over events.
      setFocusPolicy(core::FocusPolicy());
    }

    ScrollableWidget::~ScrollableWidget() {}

    void
    ScrollableWidget::setSupport(core::SdlWidget* widget) {
      // Protect from concurrent accesses.
      Guard guard(m_propsLocker);

      // Try to retrieve the existing support widget and remove it.
      core::SdlWidget* sup = getChildOrNull<core::SdlWidget>(m_supportName);

      if (sup != nullptr) {
        removeWidget(sup);
      }

      // Now insert the input widget as a support widget for this object.
      if (widget != nullptr) {
        // Insert the input widget as child of this widget so that it gets redrawn.
        widget->setParent(this);

        // Assign the new name of the scroll bar.
        m_supportName = widget->getName();

        // Perform the setup of the support widget
        setupSupport(widget);
      }
    }

    void
    ScrollableWidget::onControlScrolled(scroll::Orientation orientation,
                                        float min,
                                        float max)
    {
      // TODO: Handle modification of the viewport by changing its rendering area.
      log(
        "Should handle scrolling from \"" + std::to_string(static_cast<int>(orientation)) + "\" to [" +
        std::to_string(min) + " - " + std::to_string(max) + "]",
        utils::Level::Warning
      );
    }

    void
    ScrollableWidget::updatePrivate(const utils::Boxf& window) {
      // Protect from concurrent accesses.
      Guard guard(m_propsLocker);

      // Use the dedicated handler to compute the new rendering area
      // of the support widget.
      core::SdlWidget* support = nullptr;

      if (hasSupportWidget()) {
        support = getSupportWidget();
      }

      utils::Boxf newSize = onResize(window, support);

      // Assign the new size if the support widget is valid.
      if (support != nullptr) {
        postEvent(
          std::make_shared<core::engine::ResizeEvent>(
            newSize,
            support->getRenderingArea(),
            support
          )
        );
      }

      // Call the parent method to benefit from base class behavior.
      core::SdlWidget::updatePrivate(window);
    }

    bool
    ScrollableWidget::handleContentScrolling(const utils::Vector2f& /*posToFix*/,
                                             const utils::Vector2f& /*whereTo*/,
                                             const utils::Vector2i& motion)
    {
      // The goal is to make the `posToFix` coincide with the `whereTo` position.
      // Both positions should be expressed in local coordinate frame so we don't
      // need any conversion.
      // Note though that using the `posToFix` is usually not what we want to use
      // because the drag event only ends when a drop event is issued. So indeed
      // we will receive a lot of drag events (and thus call this method a lot of
      // times) with the same `posToFix` but `whereTo` position farther and
      // farther away from the `posToFix`. The `motion` is more interesting as it
      // describes the last modification of the `whereTo` position. Basically we
      // should have already made `posToFix` coincide with `whereTo - motion`.

      if (!hasSupportWidget()) {
        // Nothing to do.
        return false;
      }

      // Compute the distance between the initial position and the desired one: it
      // gives us an indication of the information of the translation to apply to
      // the support widget.
      utils::Vector2f delta(1.0f * motion.x(), 1.0f * motion.y());

      // Retrieve the current rendering area of the support widget and update its
      // position with the delta to apply.
      core::SdlWidget* support = getSupportWidget();
      utils::Boxf area = support->getRenderingArea();
      utils::Sizef supportDims = area.toSize();
      utils::Boxf viewport = utils::Boxf(
        area.getCenter(),
        LayoutItem::getRenderingArea().toSize()
      );

      // Make sure that the delta does not mean displaying a non-existing part of
      // the support widget. This can be checked by verifying that the area's center
      // is still larger than half the size of the area.
      bool updated = false;
      utils::Sizef max = getPreferredSizePrivate();

      if (delta.x() < 0.0f && viewport.getLeftBound() + delta.x() >= -max.w() / 2.0f) {
        area.x() += delta.x();
        updated = true;
      }
      if (delta.x() > 0.0f && viewport.getRightBound() + delta.x() <= max.w() / 2.0f) {
        area.x() += delta.x();
        updated = true;
      }

      if (delta.y() < 0.0f && viewport.getBottomBound() + delta.y() >= -max.h() / 2.0f) {
        area.y() += delta.y();
        updated = true;
      }
      if (delta.y() > 0.0f && viewport.getTopBound() + delta.y() <= max.h() / 2.0f) {
        area.y() += delta.y();
        updated = true;
      }

      // Check if anything was updated at all.
      if (!updated) {
        return false;
      }

      // Post the resize event for the support widget if needed.
      postEvent(
        std::make_shared<core::engine::ResizeEvent>(
          area,
          support->getRenderingArea(),
          support
        )
      );

      // Emit a signal to notify listeners of the new area of the support widget
      // displayed. Note that as we want to return the area visible for the support
      // widget and not from the `ScrollableWidget` perspective we should negate 
      // the center of the area (inversion of coordinate frame).
      utils::Boxf box(
        -area.x() / supportDims.w(),
        -area.y() / supportDims.h(),
        viewport.w() / supportDims.w(),
        viewport.h() / supportDims.h()
      );

      log(
        getName() + " changed visible area to " + box.toString() + " (support: " + supportDims.toString() +
        ", visible: " + utils::Boxf(-area.getCenter(), viewport.toSize()).toString() + ")",
        utils::Level::Notice
      );

      onAreaChanged.emit(box);

      // We updated the rendering area of the support widget.
      return true;
    }

    utils::Boxf
    ScrollableWidget::onResize(const utils::Boxf& window,
                               core::SdlWidget* support)
    {
      // Check consistency: we don't know how to handle resize
      // in case the `support` widget is `null`. More precisely
      // we don't have anything to do.
      if (support == nullptr) {
        return utils::Boxf();
      }

      // We want to actualize the rendering area of the support
      // widget so that it stays the same in the display area.
      // In case no valid area is assigned to the support widget
      // yet we will try to display its top-left corner.
      utils::Boxf old = support->getRenderingArea();
      utils::Sizef hint = support->getSizeHint();

      utils::Vector2f center = old.getCenter();

      if (!old.valid()) {
        // Display the top left corner of the support widget.
        center = utils::Vector2f(
          -window.w() / 2.0f + hint.w() / 2.0f,
          window.h() / 2.0f - hint.h() / 2.0f
        );
      }

      // With the center and the expected size we can determine
      // the expected box to apply to the support widget. We now
      // need to clamp it so that we don't try to display invalid
      // areas of the support widget.
      // We will handle the right and bottom bounds last so that
      // in case the support widget is too small to occupy the
      // whole area available it gets nicely displayed on the top
      // left corner: this comes from the fact that we're indeed
      // trying to r
      utils::Boxf expected(center, window.toSize());
      utils::Boxf bounds = utils::Boxf::fromSize(hint, true);

      if (expected.getLeftBound() < bounds.getLeftBound()) {
        expected.x() += (bounds.getLeftBound() - expected.getLeftBound());
      }
      if (expected.getRightBound() > bounds.getRightBound()) {
        expected.x() -= (expected.getRightBound() - bounds.getRightBound());
      }

      if (expected.getBottomBound() < bounds.getBottomBound()) {
        expected.y() += (bounds.getBottomBound() - expected.getBottomBound());
      }
      if (expected.getTopBound() > bounds.getTopBound()) {
        expected.y() -= (expected.getTopBound() - bounds.getTopBound());
      }

      // Return the built-in area.
      return utils::Boxf(expected.getCenter(), hint);
    }

    bool
    ScrollableWidget::mouseButtonPressEvent(const core::engine::MouseEvent& e) {
      // In case the button corresponds to the button used for scrolling
      // we want to assign new coordinates to follow based on the local
      // position of the mouse.
      utils::Vector2f local = mapFromGlobal(e.getMousePosition());

      if (e.getButton() != getScrollingButton()) {
        return core::SdlWidget::mouseButtonPressEvent(e);
      }

      // Protect from concurrent accesses.
      Guard guard(m_propsLocker);

      // Assign the new coordinates.
      createOrGetCoordsToFollow(local, true);

      return core::SdlWidget::mouseButtonPressEvent(e);
    }

    bool
    ScrollableWidget::mouseDragEvent(const core::engine::MouseEvent& e) {
      // We only want to event which started inside this widget. Indeed
      // the point of the drag event is to bring the point that was pointed
      // at by the mouse at the moment of the click to the current position
      // of the mouse. This allows for intuitive navigation inside a large
      // document.
      // Also we only want to react to specific buttons which are able to
      // trigger the scrolling operation.
      if (!e.getButtons().isSet(getScrollingButton())) {
        return core::SdlWidget::mouseDragEvent(e);
      }

      // Retrieve the coordinate to follow as described in the input event.
      utils::Vector2f dragStart = mapFromGlobal(e.getInitMousePosition(getScrollingButton()));
      utils::Boxf area = LayoutItem::getRenderingArea().toOrigin();

      if (!area.contains(dragStart)) {
        // The drag event did not originated from our widget, do not start
        // a scrolling operation.
        return core::SdlWidget::mouseDragEvent(e);
      }

      // Protect from concurrent accesses.
      Guard guard(m_propsLocker);

      utils::Vector2f start = createOrGetCoordsToFollow(dragStart);
      utils::Vector2f localEnd = mapFromGlobal(e.getMousePosition());

      // Call the dedicated handler to do the necessary work in order to
      // handle scrolling: if the return value indicates that some changes
      // where made to this widget we should issue a repaint.
      if (handleContentScrolling(start, localEnd, e.getMove())) {
        requestRepaint();
      }

      // Use the base handler to provide the return value.
      return core::SdlWidget::mouseDragEvent(e);
    }

    bool
    ScrollableWidget::repaintEvent(const core::engine::PaintEvent& e) {
      // We want to filter out some events produced by the support widget so
      // that we don't try to repaint areas which are outside of this item.
      // Indeed as we're handling an area larger than the parent scrollable
      // widget, we *will* receive the entirety of the support widget as a
      // repaint area.

      // Protect from concurrent accesses.
      Guard guard(m_propsLocker);

      // First check whether there is a support widget: if this is not the
      // case we are sure that we won't receive such repaint events.
      if (!hasSupportWidget()) {
        return core::SdlWidget::repaintEvent(e);
      }

      // Check whether the source of the paint event is the support widget.
      core::SdlWidget* support = getSupportWidget();

      if (!e.isEmittedBy(support)) {
        return core::SdlWidget::repaintEvent(e);
      }

      // We need to make sure that any area provided in the repaint event is
      // not larger than the dimensions of this element.
      std::vector<utils::Boxf> cropped;
      utils::Boxf thisArea = LayoutItem::getRenderingArea().toOrigin();

      const std::vector<core::engine::update::Region>& regions = e.getUpdateRegions();
      for (unsigned id = 0u ; id < regions.size() ; ++id) {
        // Convert the region to local coordinate frame.
        utils::Boxf local = (
          regions[id].frame == core::engine::update::Frame::Local ?
          regions[id].area :
          mapFromGlobal(regions[id].area)
        );

        // Check whether it is contained inside the area assigned to this widget.
        if (!thisArea.contains(local)) {
          // Only consider the intersection of the area to repaint with this area
          // to only notify changes on this area to the parent elements.
          local = thisArea.intersect(local);
        }

        cropped.push_back(mapToGlobal(local));
      }

      // Create a new repaint event from the new areas.
      core::engine::PaintEvent pe(this);
      pe.setEmitter(e.getEmitter());

      for (unsigned id = 0u ; id < cropped.size() ; ++id) {
        pe.addUpdateRegion(cropped[id]);
      }

      // Call the paint event with the newly created event.
      return core::SdlWidget::repaintEvent(pe);
    }

  }
}
