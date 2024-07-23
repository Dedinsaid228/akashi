TEMPLATE = subdirs

SUBDIRS += \
  core \
  akashi

core.file = core.pro
akashi.file = akashi.pro
akashi.depends = core
