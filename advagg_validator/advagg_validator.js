
/**
 * @file
 * Run JSHINT in the browser against the servers JS.
 */

/**
 * Have clicks to advagg_validator_js classes run JSHINT on the client.
 */
(function ($) {
  Drupal.behaviors.advagg_validator = {
    attach: function (context, settings) {
      $('.advagg_validator_js', context).click(function (context) {
        // Get Results Div.
        var results = $(this).siblings('.results');
        // Loop over each filename.
        $.each($(this).siblings('.filenames'), function() {
          var filename = $(this).val();
          try {
            var x = jQuery.ajax({
              url: settings.basePath + filename,
              dataType: 'text',
              async: false
            });
            if (JSHINT(x.responseText, Drupal.settings.jshint, Drupal.settings.jshint.predef)) {
              $(results).append('<h4>' + filename + ' Passed!</h4>');
            } else {
              $(results).append('<p><h4>' + filename + ' Failed!</h4>');
              $(results).append('<ul>');
              for (i = 0; i < JSHINT.errors.length; i++) {
                $(results).append('<li><b>' + JSHINT.errors[i].line + ':</b> ' + JSHINT.errors[i].reason + '</li>');
              }
              $(results).append('</ul></p>');
            }
          }
          catch (err) {
            $(results).append(err);
          }
        });

        return false;
      });
    }
  };
}(jQuery));
