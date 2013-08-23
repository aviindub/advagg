<?php

/**
 * @file
 * Advanced CSS/JS aggregation modifier module.
 */

/**
 * Implement hook_advagg_get_css_file_contents_alter().
 *
 * Used to run strings inside of quotes of the content attribute through the t
 * function.
 */
function advagg_mod_advagg_get_css_file_contents_alter(&$contents, $filename, $aggregate_settings) {
  if (empty($aggregate_settings['variables']['advagg_mod_css_translate'])) {
    return;
  }

  // Regexp to match double quoted strings.
  $double_quot = '"[^"\\\\]*(?:\\\\.[^"\\\\]*)*"';
  // Regexp to match single quoted strings.
  $single_quot = "'[^'\\\\]*(?:\\\\.[^'\\\\]*)*'";
  // Extract all content inside of quotes.
  $css_content_pattern = "/content:.*?($double_quot|$single_quot).*?(?:\\;|\\})/";

  // Run strings inside of quotes of the content attribute through the t
  // function.
  $contents = preg_replace_callback($css_content_pattern, 'advagg_mod_advagg_css_content_t_replace_callback', $contents);
}

/**
 * Run preg matches through the t() function.
 *
 * @param $matches
 *   Array of matches from preg_replace_callback().
 * @return
 *   Replaced String.
 */
function advagg_mod_advagg_css_content_t_replace_callback($matches) {
  // Remove quotes for t function.
  $before = substr($matches[1], 1, -1);
  // Run t function.
  $after = t($before);
  // Put back.
  return str_replace($matches[1], str_replace($before, $after, $matches[1]), $matches[0]);
}
