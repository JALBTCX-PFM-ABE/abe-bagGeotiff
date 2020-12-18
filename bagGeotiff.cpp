
/*********************************************************************************************

    This is public domain software that was developed by or for the U.S. Naval Oceanographic
    Office and/or the U.S. Army Corps of Engineers.

    This is a work of the U.S. Government. In accordance with 17 USC 105, copyright protection
    is not available for any work of the U.S. Government.

    Neither the United States Government, nor any employees of the United States Government,
    nor the author, makes any warranty, express or implied, without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes any liability or
    responsibility for the accuracy, completeness, or usefulness of any information,
    apparatus, product, or process disclosed, or represents that its use would not infringe
    privately-owned rights. Reference herein to any specific commercial products, process,
    or service by trade name, trademark, manufacturer, or otherwise, does not necessarily
    constitute or imply its endorsement, recommendation, or favoring by the United States
    Government. The views and opinions of authors expressed herein do not necessarily state
    or reflect those of the United States Government, and shall not be used for advertising
    or product endorsement purposes.
*********************************************************************************************/


/****************************************  IMPORTANT NOTE  **********************************

    Comments in this file that start with / * ! or / / ! are being used by Doxygen to
    document the software.  Dashes in these comment blocks are used to create bullet lists.
    The lack of blank lines after a block of dash preceeded comments means that the next
    block of dash preceeded comments is a new, indented bullet list.  I've tried to keep the
    Doxygen formatting to a minimum but there are some other items (like <br> and <pre>)
    that need to be left alone.  If you see a comment that starts with / * ! or / / ! and
    there is something that looks a bit weird it is probably due to some arcane Doxygen
    syntax.  Be very careful modifying blocks of Doxygen comments.

*****************************************  IMPORTANT NOTE  **********************************/



#include "bagGeotiff.hpp"
#include "bagGeotiffHelp.hpp"


double settings_version = 1.0;


bagGeotiff::bagGeotiff (int32_t *argc, char **argv, QWidget *parent)
  : QWizard (parent, 0)
{
  QResource::registerResource ("/icons.rcc");


  //  Set the main icon

  setWindowIcon (QIcon (":/icons/bagGeotiffWatermark.png"));


  //  Override the HDF5 version check so that we can read BAGs created with an older version of HDF5.

  putenv ((char *) "HDF5_DISABLE_VERSION_CHECK=2");


  //  Get the sample data for the color and sunshade examples.

  options.sample_pixmap = QPixmap (SAMPLE_WIDTH, SAMPLE_HEIGHT);
  uint8_t idata[2];
  QFile *dataFile = new QFile (":/icons/data.dat");
  options.sample_min = 99999.0;
  options.sample_max = -99999.0;

  if (dataFile->open (QIODevice::ReadOnly))
    {
      for (int32_t i = 0 ; i < SAMPLE_HEIGHT ; i++)
        {
          for (int32_t j = 0 ; j < SAMPLE_WIDTH ; j++)
            {
              dataFile->read ((char *) idata, 2);
              options.sample_data[i][j] = idata[1] * 256 + idata[0];

              options.sample_min = qMin ((float) options.sample_data[i][j], options.sample_min);
              options.sample_max = qMax ((float) options.sample_data[i][j], options.sample_max);
            }
        }
      dataFile->close ();
    }


  //  Get the user's defaults if available

  envin (&options);


  // Set the application font

  QApplication::setFont (options.font);


  setWizardStyle (QWizard::ClassicStyle);


  setOption (HaveHelpButton, true);
  setOption (ExtendedWatermarkPixmap, false);

  connect (this, SIGNAL (helpRequested ()), this, SLOT (slotHelpClicked ()));


  area_file_name = tr ("NONE");


  //  Set the window size and location from the defaults

  this->resize (options.window_width, options.window_height);
  this->move (options.window_x, options.window_y);


  setPage (0, new startPage (argc, argv, this, &options));

  setPage (1, new imagePage (this, &options));

  setPage (2, new runPage (this, &progress, &checkList));


  setButtonText (QWizard::CustomButton1, tr("&Run"));
  setOption (QWizard::HaveCustomButton1, true);
  button (QWizard::CustomButton1)->setToolTip (tr ("Start generating the GeoTIFF(s)"));
  button (QWizard::CustomButton1)->setWhatsThis (runText);
  connect (this, SIGNAL (customButtonClicked (int)), this, SLOT (slotCustomButtonClicked (int)));


  setStartId (0);
}


bagGeotiff::~bagGeotiff ()
{
}



void bagGeotiff::initializePage (int id)
{
  button (QWizard::HelpButton)->setIcon (QIcon (":/icons/contextHelp.png"));
  button (QWizard::CustomButton1)->setEnabled (false);


  switch (id)
    {
    case 0:
      break;

    case 1:
      break;

    case 2:
      button (QWizard::CustomButton1)->setEnabled (true);

      bag_file_name = field ("bag_file_edit").toString ();
      area_file_name = field ("area_file_edit").toString ();
      output_file_name = field ("output_file_edit").toString ();


      //  Save the output directory.  It might have been input manually instead of browsed.

      options.output_dir = QFileInfo (output_file_name).absoluteDir ().absolutePath ();

      options.azimuth = field ("sunAz").toDouble ();
      options.elevation = field ("sunEl").toDouble ();
      options.exaggeration = field ("sunEx").toDouble ();
      options.saturation = field ("satSpin").toDouble ();
      options.value = field ("valSpin").toDouble ();
      options.start_hsv = field ("startSpin").toDouble ();
      options.end_hsv = field ("endSpin").toDouble ();
      options.transparent = field ("transparent_check").toBool ();
      options.caris = field ("caris_check").toBool ();
      options.restart = field ("restart_check").toBool ();


      //  Use frame geometry to get the absolute x and y.

      QRect tmp = this->frameGeometry ();
      options.window_x = tmp.x ();
      options.window_y = tmp.y ();


      //  Use geometry to get the width and height.

      tmp = this->geometry ();
      options.window_width = tmp.width ();
      options.window_height = tmp.height ();


      //  Save the options.

      envout (&options);


      QString string;

      checkList->clear ();

      string = tr ("Input BAG file : ") + bag_file_name;
      checkList->addItem (string);

      string = tr ("Output file(s) : ") + output_file_name;
      checkList->addItem (string);

      if (!area_file_name.isEmpty ())
        {
          string = tr ("Area file : ") + area_file_name;
          checkList->addItem (string);
        }


      switch (options.transparent)
        {
        case false:
          string = tr ("Empty cells are blank");
          checkList->addItem (string);
          break;

        case true:
          string = tr ("Empty cells are transparent");
          checkList->addItem (string);
          break;
        }

      switch (options.caris)
        {
        case false:
          string = tr ("LZW compressed output format");
          checkList->addItem (string);
          break;

        case true:
          string = tr ("Brain-dead Caris output format");
          checkList->addItem (string);
          break;
        }

      switch (options.restart)
        {
        case false:
          string = tr ("Color map is continuous from minimum to maximum");
          checkList->addItem (string);
          break;

        case true:
          string = tr ("Color map starts over at zero boundary");
          checkList->addItem (string);
          break;
        }

      string = QString (tr ("Sun Azimuth : %1")).arg (options.azimuth, 6, 'f', 2);
      checkList->addItem (string);


      string = QString (tr ("Sun Elevation : %1")).arg (options.elevation, 5, 'f', 2);
      checkList->addItem (string);


      string = QString (tr ("Vertical Exaggeration : %1")).arg (options.exaggeration, 4, 'f', 2);
      checkList->addItem (string);


      string = QString (tr ("Color Saturation : %1")).arg (options.saturation, 4, 'f', 2);
      checkList->addItem (string);


      string = QString (tr ("Color Value : %1")).arg (options.value, 4, 'f', 2);
      checkList->addItem (string);


      string = QString (tr ("Start Hue Value : %1")).arg (options.start_hsv, 6, 'f', 2);
      checkList->addItem (string);


      string = QString (tr ("End Hue Value : %1")).arg (options.end_hsv, 6, 'f', 2);
      QListWidgetItem *cur = new QListWidgetItem (string);

      checkList->addItem (cur);
      checkList->setCurrentItem (cur);
      checkList->scrollToItem (cur);

      break;
    }
}



void bagGeotiff::cleanupPage (int id)
{
  switch (id)
    {
    case 0:
      break;

    case 1:
      break;

    case 2:
      break;
    }
}



void bagGeotiff::slotHelpClicked ()
{
  QWhatsThis::enterWhatsThisMode ();
}



//  This is where the fun stuff happens.

void 
bagGeotiff::slotCustomButtonClicked (int id __attribute__ ((unused)))
{
  int32_t             i, j, k, m, c_index, width, height, x_start, y_start, count = 0;
  float               *current_row, *next_row, min_val, max_val, range[2] = {0.0, 0.0}, shade_factor;
  double              conversion_factor, mid_y_radians, x_cell_size, y_cell_size, x_bin_size_degrees,
                      y_bin_size_degrees;
  double              polygon_x[200], polygon_y[200];
  NV_F64_XYMBR        bag_mbr, mbr;
  uint8_t             *fill, cross_zero = NVFalse;
  char                bag_file[512], name[512], area_file[512];
  QString             string;
  bagError            bagErr;
  bagHandle           bag_handle;


  QApplication::setOverrideCursor (Qt::WaitCursor);


  button (QWizard::FinishButton)->setEnabled (false);
  button (QWizard::BackButton)->setEnabled (false);
  button (QWizard::CustomButton1)->setEnabled (false);


  //  Note - The sunopts and the color_array get set in display_sample_data (imagePage.cpp).  No point in
  //  doing it twice.


  strcpy (bag_file, bag_file_name.toLatin1 ());


  //  Open the BAG file.

  if ((bagErr = bagFileOpen (&bag_handle, BAG_OPEN_READONLY, (u8 *) bag_file)) != BAG_SUCCESS)
    {
      u8 *errstr;

      if (bagGetErrorString (bagErr, &errstr) == BAG_SUCCESS)
        {
          string.sprintf (tr ("Error opening BAG file : %s").toLatin1 (), errstr);
          QMessageBox::warning (0, tr ("bagGeotiff"), string);
        }

      exit (-1);
    }

  int32_t data_cols = bagGetDataPointer (bag_handle)->def.ncols;
  int32_t data_rows = bagGetDataPointer (bag_handle)->def.nrows;
  x_bin_size_degrees = bagGetDataPointer (bag_handle)->def.nodeSpacingX;
  y_bin_size_degrees = bagGetDataPointer (bag_handle)->def.nodeSpacingY;
  bag_mbr.min_x = bagGetDataPointer (bag_handle)->def.swCornerX;
  bag_mbr.min_y = bagGetDataPointer (bag_handle)->def.swCornerY;
  bag_mbr.max_x = bag_mbr.min_x + data_cols * x_bin_size_degrees;
  bag_mbr.max_y = bag_mbr.min_y + data_rows * y_bin_size_degrees;

  //fprintf(stderr,"%s %s %d %d %d %f %f %f %f %f %f\n",__FILE__,__FUNCTION__,__LINE__,data_cols,data_rows,x_bin_size_degrees,y_bin_size_degrees,bag_mbr.min_x,bag_mbr.min_y,bag_mbr.max_x,bag_mbr.max_y);

  strcpy (name, output_file_name.toLatin1 ());

  if (strcmp (&name[strlen (name) - 4], ".tif")) strcat (name, ".tif");


  x_start = 0;
  y_start = 0;
  width = data_cols;
  height = data_rows;


  //  Check for an area file.

  mbr = bag_mbr;
  if (!area_file_name.isEmpty ())
    {
      strcpy (area_file, area_file_name.toLatin1 ());

      if (!get_area_mbr (area_file, &count, polygon_x, polygon_y, &mbr))
        {
          QString qstring = QString (tr ("Error reading area file %1\nReason : %2")).arg (area_file_name).arg (QString (strerror (errno)));
          QMessageBox::critical (this, tr ("bagGeotiff"), qstring);
          exit (-1);
        }


      if (mbr.min_y > bag_mbr.max_y || mbr.max_y < bag_mbr.min_y || mbr.min_x > bag_mbr.max_x || mbr.max_x < bag_mbr.min_x)
        {
          QString qstring = QString (tr ("Specified area is completely outside of the BAG bounds!"));
          QMessageBox::critical (this, tr ("bagGeotiff"), qstring);
          exit (-1);
        }


      //  Match to nearest cell

      x_start = NINT ((mbr.min_x - bag_mbr.min_x) / x_bin_size_degrees);
      y_start = NINT ((mbr.min_y - bag_mbr.min_y) / y_bin_size_degrees);
      width = NINT ((mbr.max_x - mbr.min_x) / x_bin_size_degrees);
      height = NINT ((mbr.max_y - mbr.min_y) / y_bin_size_degrees);


      //  Adjust to BAG bounds if necessary

      if (x_start < 0) x_start = 0;
      if (y_start < 0) y_start = 0;
      if (x_start + width > data_cols) width = data_cols - x_start;
      if (y_start + height > data_rows) height = data_rows - y_start;


      //  Redefine bounds

      mbr.min_x = bag_mbr.min_x + x_start * x_bin_size_degrees;
      mbr.min_y = bag_mbr.min_y + y_start * y_bin_size_degrees;
      mbr.max_x = mbr.min_x + width * x_bin_size_degrees;
      mbr.max_y = mbr.min_y + height * y_bin_size_degrees;
    }

  progress.mbar->setRange (0, height);


  //  Compute cell sizes for sunshading.

  mid_y_radians = (bag_mbr.max_y - bag_mbr.min_y) * 0.0174532925199432957692;
  conversion_factor = cos (mid_y_radians);
  x_cell_size = x_bin_size_degrees * 111120.0 * conversion_factor;
  y_cell_size = y_bin_size_degrees * 111120.0;


  uint8_t *red = (uint8_t *) calloc (width, sizeof (uint8_t));
  uint8_t *green = (uint8_t *) calloc (width, sizeof (uint8_t));
  uint8_t *blue = (uint8_t *) calloc (width, sizeof (uint8_t));
  uint8_t *alpha = (uint8_t *) calloc (width, sizeof (uint8_t));


  next_row = (float *) calloc (width, sizeof (float));
  current_row = (float *) calloc (width, sizeof (float));


  //  If the calloc failed, error out.
    
  if (current_row == NULL)
    {
      perror (__FILE__);
      exit (-1);
    }



  min_val = 999999999.0;
  max_val = -999999999.0;

  for (i = 0, m = 1 ; i < height ; i++, m++)
    {
      bagErr = bagReadRow (bag_handle, y_start + i, x_start, width - 1, Elevation, (void *) current_row);

      for (j = 0 ; j < width ; j++)
        {
          if (current_row[j] != NULL_ELEVATION)
            {
              float val = -current_row[j];
              if (min_val > val) min_val = val;
              if (max_val < val) max_val = val;
            }
        }

      progress.mbar->setValue (m);

      qApp->processEvents ();
    }


  progress.mbar->setValue (height);



  progress.gbar->setRange (0, height);


  if (options.restart && min_val < 0.0)
    {
      range[0] = -min_val;
      range[1] = max_val;

      cross_zero = NVTrue;
    }
  else
    {
      range[0] = max_val - min_val;

      cross_zero = NVFalse;
    }


  fill = (uint8_t *) calloc (width, sizeof (uint8_t));
  if (fill == NULL)
    {
      perror (tr ("Allocating fill").toLatin1 ());
      exit (-1);
    }



  OGRSpatialReference ref;
  GDALDataset         *df;
  char                *wkt = NULL;
  GDALRasterBand      *bd[4];
  double              trans[6];
  GDALDriver          *gt;
  char                **papszOptions = NULL;


  //  Set up the output GeoTIFF file.

  GDALAllRegister ();

  gt = GetGDALDriverManager ()->GetDriverByName ("GTiff");
  if (!gt)
    {
      QMessageBox::critical (0, tr ("bagGeotiff"), tr ("Could not get GTiff driver!"));
      exit (-1);
    }

  int32_t bands = 3;
  if (options.transparent) bands = 4;


  //  Stupid Caris software can't read normal files!

  if (options.caris)
    {
      papszOptions = CSLSetNameValue (papszOptions, "COMPRESS", "PACKBITS");
    }
  else
    {
      papszOptions = CSLSetNameValue (papszOptions, "TILED", "NO");
      papszOptions = CSLSetNameValue (papszOptions, "COMPRESS", "LZW");
    }

  df = gt->Create (name, width, height, bands, GDT_Byte, papszOptions);
  if (df == NULL)
    {
      string.sprintf (tr ("Could not create %s").toLatin1 (), name);
      QMessageBox::critical (0, tr ("bagGeotiff"), string);
      exit (-1);
    }

  trans[0] = mbr.min_x;
  trans[1] = x_bin_size_degrees;
  trans[2] = 0.0;
  trans[3] = mbr.max_y;
  trans[4] = 0.0;
  trans[5] = -y_bin_size_degrees;
  df->SetGeoTransform (trans);
  ref.SetWellKnownGeogCS ("EPSG:4326");
  ref.exportToWkt (&wkt);
  df->SetProjection (wkt);
  CPLFree (wkt);
  for (i = 0 ; i < bands ; i++) bd[i] = df->GetRasterBand (i + 1);


  for (i = height - 1, k = 0 ; i >= 0 ; i--, k++)
    {
      if (i == (height - 1))
        {
          bagErr = bagReadRow (bag_handle, y_start + i, x_start, width - 1, Elevation, (void *) current_row);

          for (j = 0 ; j < width ; j++) current_row[j] = -current_row[j];

          memcpy (next_row, current_row, width * sizeof (float));
        }
      else
        {
          memcpy (current_row, next_row, width * sizeof (float));

          bagErr = bagReadRow (bag_handle, y_start + i, x_start, width - 1, Elevation, (void *) next_row);

          for (j = 0 ; j < width ; j++) next_row[j] = -next_row[j];
        }


      memset (fill, 1, width);


      for (j = 0 ; j < width ; j++)
        {
          if (current_row[j] != -NULL_ELEVATION)
            {
              float val = current_row[j];

              if (cross_zero)
                {
                  if (val < 0.0)
                    {
                      c_index = (int32_t) (NUMHUES - (int32_t) (fabsf ((val - min_val) / range[0] * NUMHUES))) * NUMSHADES;
                    }
                  else
                    {
                      c_index = (int32_t) (NUMHUES - (int32_t) (fabsf (val) / range[1] * NUMHUES)) * NUMSHADES;
                    }
                }
              else
                {
                  c_index = (int32_t) (NUMHUES - (int32_t) (fabsf ((val - min_val) / range[0] * NUMHUES))) * NUMSHADES;
                }
            }
          else
            {
              c_index = -2; 
            }

          shade_factor = sunshade (next_row, current_row, j, &options.sunopts, x_cell_size, y_cell_size);

          if (shade_factor < 0.0) shade_factor = options.sunopts.min_shade;

          c_index -= NINT (NUMSHADES * shade_factor + 0.5);


          if (fill[j] && c_index >= 0)
            {
              red[j] = options.color_array[c_index].red ();
              green[j] = options.color_array[c_index].green ();
              blue[j] = options.color_array[c_index].blue ();
              alpha[j] = 255;
            }
          else
            {
              red[j] = green[j] = blue[j] = alpha[j] = 0;
            }
        }

      CPLErr err = bd[0]->RasterIO (GF_Write, 0, k, width, 1, red, width, 1, GDT_Byte, 0, 0);
      err = bd[1]->RasterIO (GF_Write, 0, k, width, 1, green, width, 1, GDT_Byte, 0, 0);
      err = bd[2]->RasterIO (GF_Write, 0, k, width, 1, blue, width, 1, GDT_Byte, 0, 0);
      if (options.transparent) err = bd[3]->RasterIO (GF_Write, 0, k, width, 1, alpha, width, 1, GDT_Byte, 0, 0);

      if (err == CE_Failure)
        {
          checkList->clear ();

          string = QString (tr ("Failed a TIFF scanline write - row %1")).arg (i);
          checkList->addItem (string);
        }


      progress.gbar->setValue (k + 1);

      qApp->processEvents ();
    }


  checkList->addItem (" ");
  checkList->addItem (" ");

  string = QString (tr ("Created TIFF file %1")).arg (name);
  checkList->addItem (string);

  string = QString (tr ("%1 rows by %2 columns")).arg (height).arg (width);
  checkList->addItem (string);


  delete df;


  free (fill);


  bagFileClose (bag_handle);


  button (QWizard::FinishButton)->setEnabled (true);
  button (QWizard::CancelButton)->setEnabled (false);


  QApplication::restoreOverrideCursor ();
  qApp->processEvents ();


  checkList->addItem (" ");
  QListWidgetItem *cur = new QListWidgetItem (tr ("Conversion complete, press Finish to exit."));

  checkList->addItem (cur);
  checkList->setCurrentItem (cur);
  checkList->scrollToItem (cur);
}



//  Get the users defaults.

void bagGeotiff::envin (OPTIONS *options)
{
  //  We need to get the font from the global settings.

#ifdef NVWIN3X
  QString ini_file2 = QString (getenv ("USERPROFILE")) + "/ABE.config/" + "globalABE.ini";
#else
  QString ini_file2 = QString (getenv ("HOME")) + "/ABE.config/" + "globalABE.ini";
#endif

  options->font = QApplication::font ();

  QSettings settings2 (ini_file2, QSettings::IniFormat);
  settings2.beginGroup ("globalABE");


  QString defaultFont = options->font.toString ();
  QString fontString = settings2.value (QString ("ABE map GUI font"), defaultFont).toString ();
  options->font.fromString (fontString);


  settings2.endGroup ();


  double saved_version = 1.0;


  // Set defaults so that if keys don't exist the parameters are defined

  options->transparent = NVFalse;
  options->caris = NVFalse;
  options->restart = NVTrue;
  options->azimuth = 30.0;
  options->elevation  = 30.0;
  options->exaggeration = 2.5;
  options->saturation = 1.0;
  options->value = 0.0;
  options->start_hsv = 0.0;
  options->end_hsv = 240.0;
  options->window_x = 0;
  options->window_y = 0;
  options->window_width = 640;
  options->window_height = 200;


  //  Get the INI file name

#ifdef NVWIN3X
  QString ini_file = QString (getenv ("USERPROFILE")) + "/ABE.config/bagGeotiff.ini";
#else
  QString ini_file = QString (getenv ("HOME")) + "/ABE.config/bagGeotiff.ini";
#endif

  QSettings settings (ini_file, QSettings::IniFormat);
  settings.beginGroup ("bagGeotiff");

  saved_version = settings.value (tr ("settings version"), saved_version).toDouble ();


  //  If the settings version has changed we need to leave the values at the new defaults since they may have changed.

  if (settings_version != saved_version) return;


  options->transparent = settings.value (tr ("transparent"), options->transparent).toBool ();

  options->caris = settings.value (tr ("caris format"), options->caris).toBool ();

  options->restart = settings.value (tr ("restart"), options->restart).toBool ();

  options->azimuth = (float) settings.value (tr ("azimuth"), (double) options->azimuth).toDouble ();

  options->elevation = (float) settings.value (tr ("elevation"), (double) options->elevation).toDouble ();

  options->exaggeration = (float) settings.value (tr ("exaggeration"), (double) options->exaggeration).toDouble ();

  options->saturation = (float) settings.value (tr ("saturation"), (double) options->saturation).toDouble ();

  options->value = (float) settings.value (tr ("value"), (double) options->value).toDouble ();

  options->start_hsv = (float) settings.value (tr ("start_hsv"), (double) options->start_hsv).toDouble ();

  options->end_hsv = (float) settings.value (tr ("end_hsv"), (double) options->end_hsv).toDouble ();

  options->window_width = settings.value (tr ("width"), options->window_width).toInt ();
  options->window_height = settings.value (tr ("height"), options->window_height).toInt ();
  options->window_x = settings.value (tr ("x position"), options->window_x).toInt ();
  options->window_y = settings.value (tr ("y position"), options->window_y).toInt ();

  settings.endGroup ();
}




//  Save the users defaults.

void bagGeotiff::envout (OPTIONS *options)
{
  //  Get the INI file name

#ifdef NVWIN3X
  QString ini_file = QString (getenv ("USERPROFILE")) + "/ABE.config/bagGeotiff.ini";
#else
  QString ini_file = QString (getenv ("HOME")) + "/ABE.config/bagGeotiff.ini";
#endif

  QSettings settings (ini_file, QSettings::IniFormat);
  settings.beginGroup ("bagGeotiff");


  settings.setValue (tr ("settings version"), settings_version);

  settings.setValue (tr ("transparent"), options->transparent);

  settings.setValue (tr ("caris format"), options->caris);

  settings.setValue (tr ("restart"), options->restart);

  settings.setValue (tr ("azimuth"), (double) options->azimuth);

  settings.setValue (tr ("elevation"), (double) options->elevation);

  settings.setValue (tr ("exaggeration"), (double) options->exaggeration);

  settings.setValue (tr ("saturation"), (double) options->saturation);

  settings.setValue (tr ("value"), (double) options->value);

  settings.setValue (tr ("start_hsv"), (double) options->start_hsv);

  settings.setValue (tr ("end_hsv"), (double) options->end_hsv);

  settings.setValue (tr ("width"), options->window_width);
  settings.setValue (tr ("height"), options->window_height);
  settings.setValue (tr ("x position"), options->window_x);
  settings.setValue (tr ("y position"), options->window_y);

  settings.endGroup ();
}
