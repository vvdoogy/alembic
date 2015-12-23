# Introduction #

The Alembic library is designed to be a reliable production library both today and in the future. The Alembic  development team makes the following promises to the community:

# Alembic Promises #

  1. Alembic Data will be backwards compatible with any version of the API.
  1. Read performance will be the same or improve with future versions.
  1. API will remain source code compatible unless read performance can be greatly improved.
  1. Write performance will stay the same or improve unless read performance can be greatly increased.
  1. Disk space usage will stay the same or improve unless read performance can be greatly increased.
  1. Traversing the hierarchy must remain fast and not require much data loading/memory usage to guarantee on-demand loading.
  1. Reading and writing the data for a specific node will not require the data of any other node for on-demand loading and writing.
  1. Reading and writing a frame for a node will not require any other frame for on-demand loading and writing.

Please note: _these promises can be relied on for Alembic versions 1.0 and higher._

