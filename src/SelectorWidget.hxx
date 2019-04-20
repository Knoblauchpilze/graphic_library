#ifndef    SELECTORWIDGET_HXX
# define   SELECTORWIDGET_HXX

# include "SelectorWidget.hh"
# include "SelectorLayout.hh"

namespace sdl {
  namespace graphic {

    inline
    SelectorWidget::SelectorWidget(const std::string& name,
                                   sdl::core::SdlWidget* parent,
                                   const bool transparent,
                                   const core::Palette& palette,
                                   const utils::Sizef& area):
      sdl::core::SdlWidget(name,
                           area,
                           parent,
                           transparent,
                           palette)
    {
      setLayout(std::make_shared<SelectorLayout>(0.0f, this));
    }

    inline
    void
    SelectorWidget::setActiveWidget(const std::string& name) {
      // TODO: Invalidate.
      getLayoutAs<SelectorLayout>()->setActiveItem(name);
    }

    inline
    void
    SelectorWidget::setActiveWidget(const int& index) {
      // TODO: Invalidate.
      getLayoutAs<SelectorLayout>()->setActiveItem(index);
    }

  }
}

#endif    /* SELECTORWIDGET_HXX */
