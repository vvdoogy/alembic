# About Alembic #

Alembic is an open framework for storing and sharing scene data that includes a C++ library, a file format, and client plugins and applications.  It was initially developed in 2010 by teams from Sony Pictures Imageworks and Industrial Light & Magic, and development continues today.


---

## Latest Available Release ##
2015-1-15: Alembic 1.5.8 is released. See Downloads for more details.

### Python Documentation Online! ###
We are now hosting the documentation for the python bindings at http://docs.alembic.io/python


---

## What is Alembic? ##

At the highest, most simplistic level, Alembic is "merely" a hierarchical sampled data storage format.  It is intended to be used to store a baked representation of scene data, in the same vein as GTO or OBJ.  It was designed to facilitate handoff of data between disciplines, vendors, and applications.

At a lower, less simplistic level, Alembic is designed to be very efficient with its use of memory and disk space.  Much effort has been directed towards automatic and transparent mechanisms for data de-duplication and reading/writing as little data as possible in order to fully represent the data that has been given to it.

Alembic is **not** a live scenegraph, an asset management system, a renderer, or replacement for OpenGL.

## Why was it made? ##

  * To create an open standard for scene data sharing
  * To support a baked-data workflow
    * enable easy hand-off between disciplines
    * enable faster workflows leading to greater productivity


---

# Download Alembic #

Please see directions here for GettingAlembic


---

# SIGN UP FOR ANNOUNCEMENTS #

If you are interested in being notified when updates are made, you may subscribe to the alembic-announce Google group. This is a low-traffic (1-4 messages/month) announcement list that the Alembic development team will use to communicate major and important news to interested users. The URL is:

http://groups.google.com/group/alembic-announce