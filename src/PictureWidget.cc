
# include "PictureWidget.hh"

namespace sdl {
  namespace graphic {

    PictureWidget::PictureWidget(const std::string& name,
                                       const std::string& picture,
                                       const Mode& mode,
                                       SdlWidget* parent,
                                       const core::engine::Color& color,
                                       const utils::Sizef& area):
      core::SdlWidget(name, area, parent, color),
      m_file(picture),
      m_mode(mode),
      m_picture(),
      m_picChanged(true)
    {}

    PictureWidget::~PictureWidget() {
      clearPicture();
    }

    void
    PictureWidget::drawContentPrivate(const utils::Uuid& uuid) const {
      // Load the picture.
      if (pictureChanged()) {
        loadPicture();
        m_picChanged = false;
      }

      // Compute the blit position of the picture so that it is centered.
      if (m_picture.valid()) {
        // Perform the copy operation according to the display mode.
        if (m_mode == Mode::Crop) {
          utils::Sizei sizePic = getEngine().queryTexture(m_picture);
          utils::Sizei sizeEnv = getEngine().queryTexture(uuid);

          utils::Boxf dstRect(
            (sizeEnv.w() - sizePic.w()) / 2.0f,
            (sizeEnv.h() - sizePic.h()) / 2.0f,
            sizePic.w(),
            sizePic.h()
          );

          getEngine().drawTexture(m_picture, &uuid, &dstRect);
        }

        if (m_mode == Mode::Fit) {
          getEngine().drawTexture(m_picture, &uuid, nullptr);
        }
      }
    }

  }
}
