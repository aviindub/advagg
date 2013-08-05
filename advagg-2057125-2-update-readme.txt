diff --git a/README.txt b/README.txt
index cd739ca..39da542 100644
--- a/README.txt
+++ b/README.txt
@@ -12,12 +12,13 @@ CONTENTS OF THIS FILE
  * JavaScript Bookmarklet
  * Technical Details & Hooks
  * nginx Configuration
+ * Troubleshooting
 
 
 FEATURES & BENEFITS
 -------------------
 
-Advanced CSS/JS Aggregation Core Module:
+Advanced CSS/JS Aggregation core features:
  * On demand generation of CSS/JS Aggregates. If the file doesn't exist it will
    be generated on demand.
  * Stampede protection for CSS and JS aggregation. Uses locking so multiple
@@ -29,7 +30,6 @@ Advanced CSS/JS Aggregation Core Module:
  * Smarter cache flushing. Scans all CSS/JS files that have been added to any
    aggregate; if that file has changed then flush the correct caches so the
    changes go out. The new name ensures changes go out when using CDNs.
- * Footer JS gets aggregated as well.
  * One can add JS to any region of the theme & have it aggregated.
  * Url query string to turn off aggregation for that request. ?advagg=0 will
    turn off file aggregation if the user has the "bypass advanced aggregation"
@@ -40,15 +40,30 @@ Advanced CSS/JS Aggregation Core Module:
  * Gzip support. All aggregated files can be pre-compressed into a .gz file and
    served from Apache. This is faster then gzipping the file on each request.
 
-Advanced CSS/JS Aggregation Submodules:
- * CSS/JS CDN. Uses the Google Libraries API to serve jQuery & jQuery UI from
-   the google CDN.
- * CSS/JS Compress. Can compress/minifiy files and inline CSS/JS.
- * Bundler. Given a target number of CSS/JS aggregates, this will try very hard
-   to meet that goal. It smartly groups files together.
- * Modifier. Has various tweaks packaged up. Force preprocessing for all CSS/JS;
-   move JS to footer; add defer tag to all JS; Inline all CSS/JS for given
-   paths; and the use of a shared directory for a unified multisite.
+Included submodules:
+ * advagg_bundler:
+   Smartly groups files together - given a target number of CSS/JS aggregates,
+   this will try very hard to meet that goal.
+ * advagg_css_cdn:
+   Load CSS libraries from a public CDN; currently only supports Google's CDN.
+ * advagg_css_compress:
+   Compress the compiled CSS files using a 3rd party compressor; currently
+   supports YUI (included).
+ * advagg_js_cdn:
+   Load JavaScript libraries from a public CDN; currently only supports Google's
+   CDN.
+ * advagg_js_compress:
+   Compress the compiled JavaScript files using a 3rd party compressor;
+   currently supports JSMin+ (included).
+ * advagg_mod:
+   Includes additional tweaks that may not work for all sites:
+   * Force preprocessing for all CSS/JS.
+   * Move JS to footer.
+   * Add defer tag to all JS.
+   * Inline all CSS/JS for given paths.
+   * Use a shared directory for a unified multisite.
+ * advagg_validator:
+   Validate all root CSS files using jigsaw.w3.org.
 
 
 CONFIGURATION
@@ -189,3 +204,18 @@ http://drupal.org/node/1116618
       add_header Last-Modified "Wed, 20 Jan 1988 04:20:42 GMT";
       try_files  $uri @drupal;
     }
+
+
+TROUBLESHOOTING
+---------------
+
+If the core Fast 404 Pages functionality is enabled via settings.php, the
+settings must be changed in order for the on-demand file compilation to work.
+Change this:
+    $conf['404_fast_paths_exclude'] = '/\/(?:styles)\//';
+to this:
+    $conf['404_fast_paths_exclude'] = '/\/(?:styles|advagg_(cs|j)s)\//';
+
+Similarly, if the Fast_404 module is enabled, the 'fast_404_string_whitelisting'
+variable must be set inside of settings.php. Add this to your settings.php file:
+    $conf['fast_404_string_whitelisting'][] = '/advagg_';
