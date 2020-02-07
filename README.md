# Intelligent Display Symbols/Footprints for KiCad

KiCad symbols and footprints for 4- and 8-character intelligent segmented/dot-matrix alphanumeric displays like the DL1414, DL2416, HDSP-2112, PDSP1881, etc.

At this time, the library does not include symbols/footprints for the exotic/mil-spec parts.

Some parts, like the PDSP188x and HDSP-2xxx, do not have pins in all positions. To facilitate the use of regular DIP sockets and single inline pin sockets, alternative footprints have been provided for these parts that have holes in all positions. (they end with a `_DIPxx` suffix)

(Pictures courtesy of the [Vintage Technology Association](http://www.decadecounter.com/))

![DL1414](http://www.decadecounter.com/vta/pic/dl1414.jpg)

![HDSP-2112](http://www.decadecounter.com/vta/pic/hdsp2112.jpg)

![DLG3416](http://www.decadecounter.com/vta/pic/dlg3416_1.jpg)

## Test fixture

Included in the KiCad project is a tester board based on the atmega809 microcontroller. The `code` folder contains the firmware, and instructions are in `code/main.c`. A prebuilt hex file is also provided.

## References

[1982 Litronix Optoelectronics Catalog](https://archive.org/details/LitronixOptoelectronicsCatalog1982)

[1985 Siemens Optoelectronics Catalog](http://www.bitsavers.org/components/siemens/1985_Siemens_Optoelectronics_Catalog.pdf)

[1990 Siemens Optoelectronics Data Book](http://www.bitsavers.org/components/siemens/1990_Siemens_Optoelectronics_Data_Book.pdf)

[Broadcom Smart Alphanumeric Displays](https://www.broadcom.com/products/leds-and-displays/smart-alphanumeric-displays/parallel-interface/)
