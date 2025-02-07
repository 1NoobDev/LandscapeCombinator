﻿.. _image-downloader:

Image Downloader
=================

The ``ImageDownloader`` component is used in:

* the :ref:`LandscapeSpawner <landscape-spawner>` actor to download heightmaps,
* the :ref:`LandscapeTexturer <landscape-texturer>` actor download satellite (and other) images for your landscapes,
* the ``BasicImageDownloader`` actor to simply download images.

Here are the sources that are supported by ``ImageDownloader``. Feel free to suggest new sources on:
`GitHub <https://github.com/LandscapeCombinator/LandscapeCombinator/issues>`_
and I will do my best to add them.


Image Source
------------

WMS
~~~

You can download images from any WMS server using the ``Generic WMS`` source.
Enter the GetCapabilities URL of a WMS server, and then choose a layer. 
Enter the ``MinLong (Left), MaxLong (Right), MinLat (Bottom), MaxLat (Top)``
coordinates in the given coordinate system, as well as the width and height
of the image you want to download. To find coordinates, you can use the
DuckDuckGo link from the Details Panel to search for the correct
``epsg.io`` map of the layer's coordinate system. 

There are a few preset WMS servers in the ``ImageDownloader``. If you know of a
WMS server that would be good to have here, please let me know!

* `IGN <https://wxs.ign.fr/altimetrie/geoportail/r/wms?SERVICE=WMS&VERSION=1.3.0&REQUEST=GetCapabilities>`_:
  Contains high resolution (1m per pixel) heightmaps for Overseas (e.g. ``RGEALTI-MNT_PYR-ZIP_GLP_WGS84UTM20_WMS`` for Guadeloupe) and European France (``RGEALTI-MNT_PYR-ZIP_FXX_LAMB93_WMS``).
  Choose ``Width`` to be ``MaxLong - MinLong``, and ``Height`` equals ``MaxLat - MinLat`` to get the best possible resolution.
* `SHOM <https://services.data.shom.fr/INSPIRE/wms/r?service=WMS&version=1.3.0&request=GetCapabilities>`_:
  Contains hillshade elevation models, but no heightmaps.
* `USGS 3DEPElevation <https://elevation.nationalmap.gov/arcgis/services/3DEPElevation/ImageServer/WMSServer?request=GetCapabilities&service=WMS>`_:
  Contains heigthmaps for the US.
* `USGS Imagery <https://basemap.nationalmap.gov/arcgis/services/USGSImageryOnly/MapServer/WMSServer?request=GetCapabilities&service=WMS>`_:
  Contains satellite imagery for the whole world.
* `OpenStreetMap_FR <https://wms.openstreetmap.fr/wms?request=GetCapabilities&service=WMS>`_:
  Contains OpenStreetMap data.
* `Terrestris_OSM <https://ows.terrestris.de/osm/service?SERVICE=WMS&VERSION=1.1.1&REQUEST=GetCapabilities>`_:
  Contains OpenStreetMap data.


Viewfinder Panoramas
~~~~~~~~~~~~~~~~~~~~

Please :ref:`make sure<Installation>` that you have 7Z installed if you want to use Viewfinder Panoramas.

* `Viewfinder Panoramas 1" <http://viewfinderpanoramas.org/Coverage%20map%20viewfinderpanoramas_org1.htm>`_:
  Highest resolution, around 30 meters per pixel.
  In an ``ImageDownloader``, choose "Viewfinder Panoramas 1" and enter the comma-separated list of rectangles (e.g. L31, L32).
* `Viewfinder Panoramas 3" <http://viewfinderpanoramas.org/Coverage%20map%20viewfinderpanoramas_org3.htm>`_:
  Intermadiate resolution, around 90 meters per pixel.
  In an ``ImageDownloader``, choose "Viewfinder Panoramas 3" and enter the comma-separated list of rectangles (e.g. L31, L32).
* `Viewfinder Panoramas 15" <http://viewfinderpanoramas.org/Coverage%20map%20viewfinderpanoramas_org15.htm>`_:
  Lowest resolution, around 450 meters per pixel.
  In an ``ImageDownloader``, choose "Viewfinder Panoramas 15" and enter the comma-separated list of rectangles (e.g. 15-A, 15-B, 15-G, 15-H).


USGS 1/3"
~~~~~~~~~

This is 1/3" or around 10 meters per pixel data for the United States.

* Go to the `USGS National Map Data Download Application <https://apps.nationalmap.gov/downloader/>`_.
* In the Datasets tab, click "Elevation Products (3DEP)". Unselect everything except "1/3 arc-second DEM" and "Current" underneath.
* Zoom on the area for which you want to download heightmaps.
* Click on "Search Products".
* In the "Products" tab, you can visualize the tiles that will be downloaded.
* If you want all the tiles suggested here, click on the TXT button (not CSV) to download the list of links.
* If you want only some of the tiles, add the ones that you want to your Cart, and click the TXT button here to download the list of links.
* In the Details Panel of an ``ImageDownloader``, choose ``USGS One Third`` as a source, and enter the path to the TXT file on your computer.


swissALTI3D (©swisstopo)
~~~~~~~~~~~~~~~~~~~~~~~~

This is very high resolution data, 0.5 meter per pixel, for Switzerland. It uses EPSG 2056.

* Go to the `swisstopo website <https://www.swisstopo.admin.ch/en/geodata/height/alti3d.html>`_.
* In Section "swissALTI3D - Access to geodata", choose "Selection by rectangle".
* Zoom on the map to the area that you wish to download.
* Click on "New rectangle", and draw a rectangle in the area that you want to download.
* Choose the resolution, 0.5m per pixel is the best resolution but will lead to heavy heightmaps.
* Click on "Search".
* Click on "Export all links" and then on "File ready. Click here to download".
* In the Details Panel of an ``ImageDownloader``, choose ``Swiss ALTI 3D`` as a source, and enter the path to the CSV file on your computer.


Litto 3D Guadeloupe
~~~~~~~~~~~~~~~~~~~

Please :ref:`make sure<Installation>` that you have 7Z installed if you want to use Litto 3D Guadeloupe.
This is very high resolution data, 1 meter per pixel, for Guadeloupe. It uses EPSG 4559.

* Go to the `Litto 3D Guadeloupe website <https://diffusion.shom.fr/litto3d-guad2016.html>`_.
* Click on "Télécharger".
* Click on the areas that you want to download.
* Click on "Télécharger la sélection".
* Move all the 7z files that you have downloaded into a new folder.
* In the Details Panel of an ``ImageDownloader``, choose ``Litto 3D Guadeloupe`` as a source, and enter the path to the new folder on your computer.


Local File
~~~~~~~~~~

Enter the path to a georeferenced file on your computer.


Local Folder
~~~~~~~~~~~~

Enter the path to a folder containing files named following the ``_x0_y0`` convention.


URL
~~~

Enter an URL to a georeferenced heightmap.


Preprocessing
-------------

You can preprocess downloaded heightmaps using the following options.
(These options are also available in the ``HeightmapModifier`` component that is attached to created landscapes).

* **Preprocess (bool)**:
  Check this option if you want to run an external binary to prepare the heightmaps right after fetching them.

* **Command (FString)**:
  Enter the name of the binary, which should be in your ``PATH``, and which will be used on your heightmap.
  Your processing command must take exactly two arguments: the input file and the output file.


Resolution Scaling
------------------

* **Change Resolution (bool)**:
  Check this option if you want to scale your heightmap resolution up or down using GDAL.

* **Precision Percent (int)**:
  Depending on the sizes of your heightmaps, you can use a value under ``100%`` to make
  importing the landscape faster. For heightmaps which are low resolution, you can use a value
  above ``100%`` in order to have a better landscape grid size within Unreal Engine to be able
  to paint or sculpt the landscape. Upscaling will however not add details that were not there
  in the original heightmaps.
