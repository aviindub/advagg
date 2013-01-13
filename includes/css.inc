<?php

function advagg_process_html_css(&$variables) {
  // dpm($variables);
  return;
  $schema = advagg_get_server_schema();
  $css_var = $variables['css'];
  $css_orig = $css_var;
  if (!variable_get('advagg_only_css_from_variables', ADVAGG_ONLY_CSS_FROM_VARIABLES)) {
    $css_func = drupal_add_css(CSS_DEFAULT);
    advagg_css_array_fixer($css_func);
  }
  else {
    $css_func = array();
  }
  $css = array_merge($css_var, $css_func);
  $css_func_inline = advagg_add_css_inline();
  if (!empty($css_func_inline)) {
    $css = advagg_merge_inline_css($css, $css_func_inline);
  }
  $css_conditional_styles = !empty($variables['conditional_styles']) ? $variables['conditional_styles'] : '';
  $css_styles = $variables['styles'];

  // Try cache.
  if (variable_get('advagg_use_full_cache', ADVAGG_USE_FULL_CACHE)) {
    // Build the cache ID
    // md5 of the CSS input
    // http or https
    // hostname
    // the js rendering function
    // css/js query string
    $cid = 'advagg_processor:css:'
      . md5(serialize(array($css, $css_conditional_styles))) . ':'
      . $schema . ':'
      . $_SERVER['HTTP_HOST'] . ':'
      . variable_get('advagg_css_render_function', ADVAGG_CSS_RENDER_FUNCTION) . ':'
      . substr(variable_get('css_js_query_string', '0'), 0, 1);
    $cache = cache_get($cid, 'cache_advagg_bundle_reuse');
  }
  elseif (isset($cid)) {
    unset($cid);
  }
  if (!empty($cache->data)) {
    $variables['styles'] = $cache->data;
  }
  else {
    // Build HTML code.
    $processed_css = advagg_process_css($css);
    if (!empty($processed_css)) {
      $variables['styles'] = $processed_css;
    }
    $variables['styles'] .= $css_conditional_styles;

    // Cache output.
    if (isset($cid) && variable_get('advagg_use_full_cache', ADVAGG_USE_FULL_CACHE) && lock_acquire($cid)) {
      cache_set($cid, $variables['styles'], 'cache_advagg_bundle_reuse', CACHE_TEMPORARY);
      lock_release($cid);
    }
  }
}

/**
 * Returns a themed representation of all stylesheets that should be attached to
 * the page.
 *
 * @see drupal_get_css()
 *
 * It loads the CSS in order, with 'module' first, then 'theme' afterwards.
 * This ensures proper cascading of styles so themes can easily override
 * module styles through CSS selectors.
 *
 * Themes may replace module-defined CSS files by adding a stylesheet with the
 * same filename. For example, themes/garland/system-menus.css would replace
 * modules/system/system-menus.css. This allows themes to override complete
 * CSS files, rather than specific selectors, when necessary.
 *
 * If the original CSS file is being overridden by a theme, the theme is
 * responsible for supplying an accompanying RTL CSS file to replace the
 * module's.
 *
 * @param $css
 *   (optional) An array of CSS files. If no array is provided, the default
 *   stylesheets array is used instead.
 * @param $noagg
 *   (optional) Bool indicating that aggregation should be disabled if TRUE.
 * @return
 *   A string of XHTML CSS tags.
 */
function advagg_process_css($css = NULL, $noagg = FALSE) {
  global $conf;
  $original_css = $css;
  if (!isset($css)) {
    $css = drupal_add_css(CSS_DEFAULT);
  }
  if (empty($css)) {
    return FALSE;
  }

  // Get useful info.
  list($preprocess_css, $scheme, $query_string) = advagg_process_css_js_prep($noagg, 'css');

  // Invoke hook_advagg_css_pre_alter() to give installed modules a chance to
  // modify the data in the $javascript array if necessary.
  drupal_alter('advagg_css_pre', $css, $preprocess_css, $scheme);

  // Set variables.
  $external_no_preprocess = array();
  $module_no_preprocess = array();
  $output_no_preprocess = array();
  $output_preprocess = array();
  $theme_no_preprocess = array();
  $inline_no_preprocess = array();
  $files_included = array();
  $files_aggregates_included = array();
  $inline_included = array();

  // Process input.
  foreach ($css as $media => $types) {
    // Setup some variables
    $files_included[$media] = array();
    $files_aggregates_included[$media] = array();
    $inline_included[$media] = array();

    // If CSS preprocessing is off, we still need to output the styles.
    // Additionally, go through any remaining styles if CSS preprocessing is on
    // and output the non-cached ones.
    foreach ($types as $type => $files) {
      if ($type == 'module') {
        // Setup theme overrides for module styles.
        $theme_styles = array();
        foreach (array_keys($css[$media]['theme']) as $theme_style) {
          $theme_styles[] = basename($theme_style);
        }
      }
      foreach ($types[$type] as $file => $preprocess) {
        // If the theme supplies its own style using the name of the module
        // style, skip its inclusion. This includes any RTL styles associated
        // with its main LTR counterpart.
        if ($type == 'module' && in_array(str_replace('-rtl.css', '.css', basename($file)), $theme_styles)) {
          // Unset the file to prevent its inclusion when CSS aggregation is enabled.
          unset($types[$type][$file]);
          continue;
        }
        // If a CSS file is not to be preprocessed and it's an inline CSS blob
        // it needs to *always* appear at the *very bottom*.
        if ($type == 'inline') {
          if (is_array($preprocess)) {
            foreach ($preprocess as $suffix => $blob) {
              $blob = advagg_drupal_load_stylesheet_content($blob, $preprocess);
              // Invoke hook_advagg_css_inline_alter() to give installed modules
              // a chance to modify the contents of $blob if necessary.
              drupal_alter('advagg_css_inline', $blob);

              $inline_no_preprocess[] = array(
                'media' => $media,
                'data' => $blob,
                'prefix' => $file,
                'suffix' => $suffix,
              );
              $inline_included[$media][] = $blob;
            }
          }
          // Unset to prevent its inclusion.
          unset($types[$type][$file]);
          continue;
        }

        $prefix = '';
        $suffix = '';
        // Invoke hook_advagg_css_extra_alter() to
        // give installed modules a chance to modify the prefix or suffix for a
        // given filename.
        $values = array($file, NULL, $prefix, $suffix);
        drupal_alter('advagg_css_extra', $values);
        list($file, $null, $prefix, $suffix) = $values;

        if ($type == 'inline') {
          $file = advagg_drupal_load_stylesheet_content($file, $preprocess);
          // Invoke hook_advagg_css_inline_alter() to give installed modules a
          // chance to modify the contents of $file if necessary.
          drupal_alter('advagg_css_inline', $file);

          $inline_no_preprocess[] = array(
            'media' => $media,
            'data' => $file,
            'prefix' => $prefix,
            'suffix' => $suffix,
          );
          $inline_included[$media][] = $file;
          // Unset to prevent its inclusion.
          unset($types[$type][$file]);
          continue;
        }

        // If a CSS file is not to be preprocessed and it's an external
        // CSS file, it needs to *always* appear at the *very top*,
        // regardless of whether preprocessing is on or off.
        if ($type == 'external') {
          $external_no_preprocess[] = array(
            'media' => $media,
            'href' => $file,
            'prefix' => $prefix,
            'suffix' => $suffix,
          );
          $files_included[$media][$file] = TRUE;
          // Unset the file to prevent its inclusion.
          unset($types[$type][$file]);
          continue;
        }

        // Only include the stylesheet if it exists.
        if (advagg_file_exists($file)) {
          if (!$preprocess || !($scheme === 'public' && $preprocess_css)) {


            // Create URI for file.
            $file_uri = advagg_build_uri($file) . $query_string;
            $files_included[$media][$file] = $preprocess;
            // If a CSS file is not to be preprocessed and it's a module CSS
            // file, it needs to *always* appear at the *top*, regardless of
            // whether preprocessing is on or off.
            if (!$preprocess && $type == 'module') {
              $module_no_preprocess[] = array(
                'media' => $media,
                'href' => $file_uri,
                'prefix' => $prefix,
                'suffix' => $suffix,
              );
            }
            // If a CSS file is not to be preprocessed and it's a theme CSS
            // file, it needs to *always* appear at the *bottom*, regardless of
            // whether preprocessing is on or off.
            elseif (!$preprocess && $type == 'theme') {
              $theme_no_preprocess[] = array(
                'media' => $media,
                'href' => $file_uri,
                'prefix' => $prefix,
                'suffix' => $suffix,
              );
            }

            else {
              $output_no_preprocess[] = array(
                'media' => $media,
                'href' => $file_uri,
                'prefix' => $prefix,
                'suffix' => $suffix,
              );
            }
          }
        }
      }
    }

    if ($scheme == 'public' && $preprocess_css) {
      $files_aggregates_included[$media] = $files_included[$media];
      $files = array();
      foreach ($types as $type) {
        foreach ($type as $file => $cache) {
          if ($cache) {
            $files[] = $file;
            $files_included[$media][$file] = TRUE;
            unset($files_aggregates_included[$file]);
          }
        }
      }
      if (!empty($files)) {
        $preprocess_files = advagg_css_js_file_builder('css', $files, $query_string);
        if (!empty($preprocess_files)) {
          $good = TRUE;
          foreach ($preprocess_files as $preprocess_file => $extra) {
            // Empty aggregate, skip
            if (empty($preprocess_file)) {
              continue;
            }

            if ($extra !== FALSE && is_array($extra)) {
              $prefix = $extra['prefix'];
              $suffix = $extra['suffix'];
              $output_preprocess[] = array(
                'media' => $media,
                'href' => advagg_build_uri($preprocess_file),
                'prefix' => $prefix,
                'suffix' => $suffix,
              );
              $files_aggregates_included[$media][$preprocess_file] = $extra;
            }
            else {
              $good = FALSE;
              break;
            }
          }
        }
        if (empty($good)) {
          // Redo with aggregation turned off and return the new value.
          watchdog('advagg', 'CSS aggregation failed. %filename could not be saved correctly.', array('%filename' => $preprocess_file), WATCHDOG_ERROR);
          $data = advagg_process_css($original_css, TRUE);
          return $data;
        }
      }
    }
  }

  // Default function called: advagg_unlimited_css_builder
  $function = variable_get('advagg_css_render_function', ADVAGG_CSS_RENDER_FUNCTION);
  return $function($external_no_preprocess, $module_no_preprocess, $output_no_preprocess, $output_preprocess, $theme_no_preprocess, $inline_no_preprocess, $inline_included, $files_included, $files_aggregates_included);
}

/**
 * Logic to figure out what kind of css tags to use.
 *
 * @param $external_no_preprocess
 *   array of css files ($media, $href, $prefix, $suffix)
 * @param $module_no_preprocess
 *   array of css files ($media, $href, $prefix, $suffix)
 * @param $output_no_preprocess
 *   array of css files ($media, $href, $prefix, $suffix)
 * @param $output_preprocess
 *   array of css files ($media, $href, $prefix, $suffix)
 * @param $theme_no_preprocess
 *   array of css files ($media, $href, $prefix, $suffix)
 * @param $inline_no_preprocess
 *   array of css data to inline ($media, $data)
 * @param $inline_included
 *   array of inline css included. $a[$media][] = $datablob;
 * @param $files_included
 *   array of css files included. $a[$media][] = $filename
 * @param $files_aggregates_included
 *   array of css files & aggregates included. $a[$media][] = $filename
 * @return
 *   html for loading the css. html for the head.
 */
function advagg_unlimited_css_builder($external_no_preprocess, $module_no_preprocess, $output_no_preprocess, $output_preprocess, $theme_no_preprocess, $inline_no_preprocess, $files_included, $files_aggregates_included, $inline_included) {
  global $user;
  $styles = '';
  $files = array_merge($external_no_preprocess, $module_no_preprocess, $output_no_preprocess, $output_preprocess, $theme_no_preprocess, $inline_no_preprocess);

  // Select method for css html output
  if (count($files) < variable_get('advagg_css_count_threshold', ADVAGG_CSS_COUNT_THRESHOLD)) {
    advagg_unlimited_css_traditional($files, $styles);
  }
  elseif (variable_get('advagg_css_logged_in_ie_detect', ADVAGG_CSS_LOGGED_IN_IE_DETECT) && $user->uid != 0) {
    // Detect IE browsers here
    $is_ie = FALSE;
    if (isset($_SERVER['HTTP_USER_AGENT'])) {
      // Strings for testing found via
      // http://chrisschuld.com/projects/browser-php-detecting-a-users-browser-from-php/
      // Test for v1 - v1.5 IE
      // Test for versions > 1.5
      // Test for Pocket IE
      if (   stristr($_SERVER['HTTP_USER_AGENT'], 'microsoft internet explorer')
            || stristr($_SERVER['HTTP_USER_AGENT'], 'msie')
            || stristr($_SERVER['HTTP_USER_AGENT'], 'mspie')
          ) {
        $is_ie = TRUE;
      }
    }
    // Play Safe and treat as IE if user agent is not set
    else {
      $is_ie = TRUE;
    }

    if ($is_ie) {
      advagg_unlimited_css_import(array_merge($external_no_preprocess, $module_no_preprocess, $output_no_preprocess), $styles);
      advagg_unlimited_css_import($output_preprocess, $styles);
      advagg_unlimited_css_import($theme_no_preprocess, $styles);
      advagg_unlimited_css_traditional($inline_no_preprocess, $styles);
    }
    else {
      advagg_unlimited_css_traditional($files, $styles);
    }
  }
  else {
    advagg_unlimited_css_import(array_merge($external_no_preprocess, $module_no_preprocess, $output_no_preprocess), $styles);
    advagg_unlimited_css_import($output_preprocess, $styles);
    advagg_unlimited_css_import($theme_no_preprocess, $styles);
    advagg_unlimited_css_traditional($inline_no_preprocess, $styles);
  }

  return $styles;
}

/**
 * Use link tags for CSS
 *
 * @param $files
 *   array of css files ($media, $href, $prefix, $suffix)
 * @param &$styles
 *   html string
 */
function advagg_unlimited_css_traditional($files, &$styles) {
  $last_prefix = '';
  $last_suffix = '';
  foreach ($files as $css_file) {
    $media = $css_file['media'];
    $prefix = empty($css_file['prefix']) ? '' : $css_file['prefix'] . "\n";
    $suffix = empty($css_file['suffix']) ? '' : $css_file['suffix'];

    // Group prefixes and suffixes.
    if (isset($css_file['href'])) {
      $href = $css_file['href'];
      if ($prefix != $last_prefix) {
        $styles .= $last_suffix . "\n" . $prefix . '<link type="text/css" rel="stylesheet" media="' . $media . '" href="' . $href . '" />' . "\n";
      }
      else {
        $styles .= '<link type="text/css" rel="stylesheet" media="' . $media . '" href="' . $href . '" />' . "\n";
      }
    }
    else {
      $data = $css_file['data'];
      if ($prefix != $last_prefix) {
        $styles .= $last_suffix . "\n" . $prefix . '<style type="text/css" media="' . $media . '">' . "\n" . $data . "\n" . '</style>' . "\n";
      }
      else {
        $styles .= '<style type="text/css" media="' . $media . '">' . "\n" . $data . "\n" . '</style>' . "\n";
      }
    }
    $last_prefix = $prefix;
    $last_suffix = $suffix;
  }
  $styles .= empty($last_suffix) ? '' : $last_suffix . "\n";
}

/**
 * Use import tags for CSS
 *
 * @param $files
 *   array of css files ($media, $href)
 * @param &$styles
 *   html string
 */
function advagg_unlimited_css_import($files, &$styles) {
  $counter = 0;
  $media = NULL;
  $import = '';
  foreach ($files as $css_file) {
    $media_new = $css_file['media'];
    $href = $css_file['href'];
    if ($media_new != $media || $counter > variable_get('advagg_css_count_threshold', ADVAGG_CSS_COUNT_THRESHOLD)) {
      if ($media && !empty($import)) {
        $styles .= "\n" . '<style type="text/css" media="' . $media . '">' . "\n" . $import . '</style>';
        $import = '';
      }
      $counter = 0;
      $media = $media_new;
    }
    $import .= '@import "' . $href . '";' . "\n";
    $counter++;
  }
  if ($media && !empty($import)) {
    $styles .= "\n" . '<style type="text/css" media="' . $media . '">' . "\n" . $import . '</style>';
  }
}