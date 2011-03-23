<?php

/**
 * @file
 * Admin page callbacks for the advagg bundler module.
 */

/**
 * Page generation function for admin/settings/bundler
 */
function advagg_bundler_admin_page() {
  $output = '';
  return $output . drupal_get_form('advagg_bundler_admin_settings_form');
}

/**
 * Form builder; Configure advagg settings.
 *
 * @ingroup forms
 * @see system_settings_form()
 */
function advagg_bundler_admin_settings_form() {
  $form = array();

  $options = array(
    0 => 0,
    1 => 1,
    2 => 2,
    3 => 3,
    4 => 4,
    5 => 5,
    6 => 6,
    7 => 7,
    8 => 8,
    9 => 9,
    10 => 10,
  );

  $form['advagg_bundler_max_css'] = array(
    '#type'           => 'select',
    '#title'          => t('Max Number Of CSS Bundles Per Page'),
    '#default_value'  => variable_get('advagg_bundler_max_css', ADVAGG_BUNDLER_MAX_CSS),
    '#options'        => $options,
    '#description'    => t('If 0 is selected then the bundler is disabled'),
  );
  $form['advagg_bundler_max_js'] = array(
    '#type'           => 'select',
    '#title'          => t('Max Number Of JS Bundles Per Page'),
    '#default_value'  => variable_get('advagg_bundler_max_js', ADVAGG_BUNDLER_MAX_JS),
    '#options'        => $options,
    '#description'    => t('If 0 is selected then the bundler is disabled'),
  );

  $form['info'] = array(
    '#type'          => 'fieldset',
    '#collapsible'   => TRUE,
    '#collapsed'     => TRUE,
    '#title'         => t('Raw Grouping Info'),
  );
  module_load_include('inc', 'advagg', 'advagg.admin');
  $analysis = advagg_bundler_analysis();
  $data = array();
  foreach ($analysis as $filename => $key) {
    $data[$key][] = $filename;
  }
  asort($analysis);
  list($data, $rows) = advagg_form_print_r($data);
  $form['info']['advagg_bundler_info'] = array(
    '#type'         => 'textarea',
    '#title'        => $row['filename'],
    '#default_value'  => $data,
    '#rows'           => $rows-1,
  );

  return system_settings_form($form);
}

/**
 * Validate form values. Used to unset variables before they get saved.
 */
function advagg_bundler_admin_settings_form_validate($form, &$form_state) {
  // Remove non variable form info.
  unset($form_state['values']['advagg_bundler_info']);
}