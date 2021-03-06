#ifndef    BUTTON_HH
# define   BUTTON_HH

# include <mutex>
# include <memory>
# include <vector>
# include <sdl_core/SdlWidget.hh>

namespace sdl {
  namespace graphic {

    namespace button {

      /**
       * @brief - Various mode for a button: can describe a regular button or a toggle
       *          button which stays in a state until the user clicks again on it.
       */
      enum class Type {
        Regular,
        Toggle
      };

    }

    class Button: public core::SdlWidget {
      public:

        Button(const std::string& name,
               const std::string& text,
               const std::string& icon,
               const std::string& font,
               const button::Type& type = button::Type::Regular,
               unsigned size = 15,
               core::SdlWidget* parent = nullptr,
               float bordersSize = 10.0f,
               const utils::Sizef& area = utils::Sizef(),
               const core::engine::Color& color = core::engine::Color::NamedColor::Silver);

        virtual ~Button();

        /**
         * @brief - Reimplementation of the base `core::SdlWidget` in order to filter the
         *          returned widget if the position lies inside this button.
         *          Indeed even though the button uses some children to display its content
         *          we want to make it behave as if it was a single element.
         *          To do so we hijack the returned element if it corresponds to an ancestor
         *          of this element.
         * @param pos - a vector describing the position which should be spanned by the
         *              items.
         * @return - a valid pointer if any of the children items spans the input position
         *           and `null` otherwise.
         */
        const core::SdlWidget*
        getItemAt(const utils::Vector2f& pos) const noexcept override;

        /**
         * @brief - Used to switch the state of this button to be toggled or not based on the
         *          value of the input boolean. Note that this might mess up the events based
         *          handling of the state so use with care.
         *          Note also that this method does not have any effect if the button is not
         *          a toggle button.
         * @param toggled - `true` if the button state should be set to `Toggled` and `false`
         *                  otherwise.
         */
        void
        toggle(bool toggled);

        /**
         * @brief - Used to determine whether the button is toggled as the time of calling
         *          this method. Note that in the case of a `Regular` button this method
         *          can never return `true`.
         * @return - `true` if the button is toggled and `false` otherwise.
         */
        bool
        toggled();

      protected:

        /**
         * @brief - Reimplementation of the base class method to provide update of the borders
         *          when a resize is requested.
         * @param window - the available size to perform the update.
         */
        void
        updatePrivate(const utils::Boxf& window) override;

        /**
         * @brief - Reimplementation of the base `SdlWidget` method. This allows to draw some
         *          sort of border for this button to make it resemble to a button.
         * @param uuid - the identifier of the canvas which we can use to draw the overlay.
         * @param area - the area of the canvas to update.
         */
        void
        drawContentPrivate(const utils::Uuid& uuid,
                           const utils::Boxf& area) override;

        /**
         * @brief - Reimplementation of the base class method to handle cases where the mouse
         *          is dragged to another widget which would prevent the button to be reset in
         *          its initial state.
         * @param e - the event to be interpreted.
         * @return - `true` if the event was recognized and `false` otherwise.
         */
        bool
        dropEvent(const core::engine::DropEvent& e) override;

        /**
         * @brief - Reimplementation of the base class method to provide update of the borders
         *          when the user push on this button.
         * @param e - the event to be interpreted.
         * @return - `true` if the event was recognized and `false` otherwise.
         */
        bool
        mouseButtonPressEvent(const core::engine::MouseEvent& e) override;

        /**
         * @brief - Reimplementation of the base class method to provide update of the borders
         *          when the user release on this button.
         * @param e - the event to be interpreted.
         * @return - `true` if the event was recognized and `false` otherwise.
         */
        bool
        mouseButtonReleaseEvent(const core::engine::MouseEvent& e) override;

      private:

        /**
         * @brief- Convenience structure describing the information to pass on to create the
         *         label widget to display the text of the button.
         */
        struct TextData {
          std::string title;
          std::string font;
          unsigned size;
        };

        /**
         * @brief - Used to retrieve the maximum size available for an icon in a button. This
         *          size is assigned when creating the icon for any button.
         * @return - a size describing the icon's maximum dimensions.
         */
        static
        utils::Sizef
        getIconMaxDims() noexcept;

        /**
         * @brief - Retrieves the default button for this button to be pushed.
         * @return - a default button for the click on this button.
         */
        static
        core::engine::mouse::Button
        getClickButton() noexcept;

        /**
         * @brief - Retrieves the color role to use to display borders for their first possible
         *          role. Indeed borders in button can have one of two roles based on whether
         *          the button is pushed.
         * @return - the first color role to make the borders' colors alternate.
         */
        static
        core::engine::Palette::ColorRole
        getBorderColorRole() noexcept;

        /**
         * @brief - Similar method as `getBorderColorRole` but returns the second possible color
         *          role.
         * @return - the second color role to make the borders' colors alternate.
         */
        static
        core::engine::Palette::ColorRole
        getBorderAlternateColorRole() noexcept;

        /**
         * @brief - Used to create the layout needed to represent this button.
         * @param icon - the icon to use for this button.
         * @param text - the text to display for this button.
         */
        void
        build(const std::string& icon,
              const TextData& text);

        /**
         * @brief - Describes whether the borders should be repainted.
         *          Note that the locker is assumed to already be acquired.
         * @return - `true` if the borders should be repainted and `false` otherwise.
         */
        bool
        bordersChanged() const noexcept;

        /**
         * @brief - Defines that the borders should be repainted. Also triggers a call
         *          to the repaint method from the parent class.
         *          Note that the locker should already be assumed before using this
         *          method.
         */
        void
        setBordersChanged();

        /**
         * @brief - Used to perform the loading of the borders to update the internal attributes.
         *          Note that the locker is assumed to already be acquired. The textures are not
         *          checked to determine whether we actually need a repaint.
         */
        void
        loadBorders();

        /**
         * @brief - Clears the textures representing the border of this button.
         */
        void
        clearBorders();

        /**
         * @brief - Used whenever a meaningful mouse button release event is detected. We want to
         *          toggle the button if needed and update the borders so that they are accurately
         *          reflecting the state of the button.
         *          Note that the locker is assumed to already be acquired.
         */
        void
        updateButtonState();

      private:

        /**
         * @brief - Convenience enumeration describing the current state of the button. This is
         *          useful in addition to the `pressed` status of the border to allow determine
         *          precisely the current appearance of the button.
         */
        enum class State {
          Released,
          Pressed,
          Toggled
        };

        /**
         * @brief - Convenience structure describing the internal properties to use to represent
         *          the borders for this button.
         */
        struct BordersData {
          utils::Uuid hLightBorder;
          utils::Uuid hDarkBorder;
          utils::Uuid vLightBorder;
          utils::Uuid vDarkBorder;

          float size;

          bool pressed;
        };

        /**
         * @brief - Protects concurrent accesses to the properties of this button.
         */
        mutable std::mutex m_propsLocker;

        /**
         * @brief - The type of the button. It describes the behavior of the button
         *          when it is clicked.
         */
        button::Type m_type;

        /**
         * @brief - Describes whether the borders should be recomputed or can be
         *          used as is.
         */
        bool m_bordersChanged;

        /**
         * @brief - The borders' data for this button.
         */
        BordersData m_borders;

        /**
         * @brief - The current state of the button. This state can only be changed
         *          through a complete sequence mouse button click and mouse button
         *          release occurring inside the button.
         */
        State m_state;

      public:

        /**
         * @brief - Signal used to notify external listeners that this button has bee,
         *          toggled. Note that this signal is fired in addition to the `onClick`
         *          signal from the base class but only for toggle buttons.
         *          Regular buttons will never use this signal. The parameter allows to
         *          determine whether the new state of the button is toggled.
         *          Note finally that the signal is *not* emitted when the `toggle`
         *          method is called as we suppose that this comes from a deliberate
         *          action of the user and thus we don't need to notify it (as listeners
         *          are probably already aware of that).
         */
        utils::Signal<std::string, bool> onButtonToggled;
    };

    using ButtonShPtr = std::shared_ptr<Button>;
  }
}

# include "Button.hxx"

#endif    /* BUTTON_HH */
