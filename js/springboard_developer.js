(function($) {
  Drupal.behaviors.springboard_developer = {
    attach: function(context, settings) {
      // When a checkbox's parent element is clicked.
      $(document).ready(function() {
        var $select_actions = $('.select-actions input');

        $('table.springboard-developer-edit-payment-gateways th.first, tr[id^="payment-method-"] td.first').on('click', function(e) {
          if ($(e.target).is('input[type="checkbox"]')) {
            return;
          }
          $(this).find('input[type="checkbox"]').click();
        });

        var $all_checkbox = $('table.springboard-developer-edit-payment-gateways th.first input[type="checkbox"]');
        var $checkboxes = $('table.springboard-developer-edit-payment-gateways td.first input[type="checkbox"]');
        $all_checkbox.on('change', function() {
          $checkboxes.prop('checked', !($checkboxes.length === $checkboxes.filter(':checked').length));
          $select_actions.toggleClass('disabled', !($checkboxes.filter(':checked').length > 0));
        });

        var refresh_actions = function() {
          // Only enable the "disable all" button if at least one checked
          // payment method's has a disable button in its row that is not
          // disabled.
          $select_actions.filter('.disable').toggleClass('disabled', !($checkboxes.filter(function(i, el) {
            return $(el).is(':checked') && $(el).parents('tr').find('input[id^="disable-"]:enabled').length;
          }).length));
          // Only enable the "enable all" button if at least one checked
          // payment method has an enable button in its row.
          $select_actions.filter('.enable').toggleClass('disabled', !($checkboxes.filter(function(i, el) {
            return $(el).is(':checked') && $(el).parents('tr').find('input[id^="enable-"]').length;
          }).length));
          // Always enable the other action buttons when any payment method is
          // selected.
          $select_actions.filter('.autoconfig, .autofill:visible').toggleClass('disabled', !($checkboxes.filter(':checked').length > 0));
          $all_checkbox.prop('checked', $checkboxes.length === $checkboxes.filter(':checked').length);
        };

        $checkboxes.on('change', refresh_actions);
        $(document).on('ajaxComplete', refresh_actions);

        var $parent_tr;
        $(document).on('click', '.select-actions input', function() {
          var $this = $(this);
          if ($this.hasClass('disabled')) {
            return;
          }

          var action = '';
          var class_name = '';
          if ($this.hasClass('disable')) {
            action = 0;
            class_name = 'disable-payment-method';
          }
          else if ($this.hasClass('enable')) {
            action = 1;
            class_name = 'enable-payment-method';
          }
          else if ($this.hasClass('autofill')) {
            action = 2;
            class_name = '.lp-autofill-settings';
          }
          else if ($this.hasClass('autoconfig')) {
            action = 3;
            class_name = 'auto-config-settings';
          }

          var instance_ids = [];
          var checked = $checkboxes.filter(':checked');
          var buttons_to_modify = [];
          checked.each(function(index, value) {
            $parent_tr = $(value).parents('tr');
            var $b = $parent_tr.find('input.' + class_name + ':enabled');
            if ($b.length) {
              instance_ids.push($parent_tr.data('payment-method-id'));
              buttons_to_modify.push($b);
              $b.addClass('disabled').addClass('progress-disabled').prop('disabled', true);
            }
          });
          console.log(buttons_to_modify);
          var $buttons_to_modify = $(buttons_to_modify);
          instance_ids = instance_ids.join(',');

          switch (action) {
            case 0:
            case 1:
              var able = action == 0 ? 'disable' : 'enable';
              $.get($this.data('url') + '/' + instance_ids + '/' + able, function(response) {
                var buttons = response.split(',');
                $buttons_to_modify.each(function(index, value) {
                  $(value).replaceWith(buttons[index]);
                });
              });
              break;

            case 2:
            case 3:
              var success_text = action == 2 ? 'Autofilled' : 'Autoconfigged';
              var og_text = action == 2 ? 'LP Autofill' : 'Dev Config';
              $.get($this.data('url') + '/' + instance_ids, function(response) {
                if (response.length > 0) {
                  response = $.parseJSON(response);
                  alert(response.message);
                  console.error(response.message);
                  $buttons_to_modify.each(function(i, v) {
                    $(v).val('Error').addClass('warning');
                  });
                  setTimeout(function() {
                    $buttons_to_modify.each(function(i, v) {
                      $(v).val(og_text).removeClass('warning');
                    });
                  }, 3000);
                }
                else {
                  $buttons_to_modify.each(function(i, v) {
                    var $v = $(v);
                    var $next_tr = $v.parents('tr').next('tr.payment-method-settings-row');
                    if ($next_tr.length > 0) {
                      var $button = $v.siblings('.edit-payment-method-settings');
                      $next_tr.remove();
                      $button.click();
                    }
                    $v.val(success_text).addClass('success');
                    setTimeout(function() {
                      $v.val(og_text).removeClass('success');
                    }, 3000);
                  });
                }
                $buttons_to_modify.each(function(i, v) {
                  $(v).prop('disabled', false).removeClass('progress-disabled');
                });
              });
          }
        });
      });

      // When a payment method's cancel button is clicked.
      $(document).on('click', '.remove-payment-method-setting-row', function() {
        $(this).parents('tr.payment-method-settings-row').remove();
      });

      // When a payment method's settings button is clicked.
      $('input.edit-payment-method-settings').on('click', function() {
        var $this = $(this);
        $this.prop('disabled', true).addClass('progress-disabled');
        $.get($this.data('url'), function(data) {
          $this.prop('disabled', false).removeClass('progress-disabled');
          $this.parents('tr').next('.payment-method-settings-row').remove().end().after(data);
        });
        return false;
      });

      // When a payment method's settings save button is clicked.
      $(document).on('click', 'input.save-payment-method-settings', function() {
        var $this = $(this);
        var $payment_method = $(this).data('payment-method');
        var $parentTR = $(this).parents('tr.payment-method-settings-row');
        var data = $parentTR.children('td.first').find('select, input, textarea').serialize();
        var url = $(this).data('url');
        $.post(url + '/submit', data, function(response) {
          if (response.length > 0) {
            alert(response);
            console.error(response);
          }
          else {
            var $settings_button = $parentTR.prev('tr').find('input.edit-payment-method-settings');
            $settings_button.val('Saved').addClass('success');
            setTimeout(function() {
              $settings_button.val('Settings').removeClass('success');
            }, 3000);
            $parentTR.remove();
          }
        });
        return false;
      });

      // When the reset button is clicked, confirm and display a message.
      var $reset_button = $('#springboard_reset_button');
      $reset_button.on('click', function() {
        var $this = $(this);
        $this.prop('disabled', true).addClass('progress-disabled');
        if (confirm('Are you sure you want to delete all submission data? This is irreversible!')) {
          $.get($(this).data('url'), function(response) {
            if (response.length > 0) {
              alert(response);
              console.error(response);
            }
            else {
              $('#reset-button-cleared').stop().show();
              setTimeout(function() {
                $('#reset-button-cleared').fadeOut(2000);
              }, 2000);
            }
            $this.prop('disabled', false).removeClass('progress-disabled');
          });
        }
        return false;
      });

      var $lp_login_button = $('input[name="lastpass_login_button"]');
      var $lp_logout_button = $('input[name="lastpass_logout_button"]');
      var $lp_login_fields = $('.lp-login-field input');
      // When the LP login button is clicked.
      $lp_login_button.on('click', function() {
        var serialized = $lp_login_fields.serialize();
        $lp_login_fields.add($lp_login_button).prop('disabled', true);
        $.post($lp_login_button.data('url'), serialized, function(response) {
          $lp_login_fields.add($lp_login_button).prop('disabled', false);
          response = $.parseJSON(response);
          if (response.status == 'error') {
            $('.lp-login-error').text(response.message);
          }
          else if (response.status == 'ok') {
            $('.lp-login-error').text('');
            $('body').removeClass('lp-not-logged-in').addClass('lp-logged-in');
            $('.lp-username').text(response.username);
          }
        });
        return false;
      });
      // When the LP logout button is clicked.
      $lp_logout_button.on('click', function() {
        $lp_login_fields.prop('disabled', true);
        $.get($lp_logout_button.data('url'), function(response) {
          response = $.parseJSON(response);
          if (response.status == 'ok') {
            $('body').removeClass('lp-logged-in').addClass('lp-not-logged-in');
          }
          $lp_login_fields.prop('disabled', false);
        });
        return false;
      });

      // Submit LP login when LP username/password fields have focus.
      $('#springboard-developer-admin-settings').submit(function() {
        return false;
      });
      $lp_login_fields.keypress(function(event) {
        if (event.keyCode == '13') {
          event.preventDefault();
          $('input[name="lastpass_login_button"]').click();
          return false;
        }
      });

      // When a toggle button is clicked, make the request.
      $toggle_button = $('.enable-payment-method, .disable-payment-method');
      $(document).on('click', '.enable-payment-method, .disable-payment-method', function() {
        var $this = $(this);
        $this.prop('disabled', true).addClass('progress-disabled');
        $.get($this.data('url'), function(response) {
          $this.replaceWith(response);
        });
        return false;
      });

      // When an autofill button is clicked, make the request.
      $lp_autofill = $('.lp-autofill-settings');
      $lp_autofill.on('click', function() {
        var $this = $(this);
        $this.prop('disabled', true).addClass('progress-disabled');
        $.get($this.data('url'), function(response) {
          if (response.length > 0) {
            response = $.parseJSON(response);
            alert(response.message);
            console.error(response.message);
            $this.val('Error').addClass('warning');
            setTimeout(function() {
              $this.val('LP Autofill').removeClass('warning');
            }, 3000);
          }
          else {
            var $next_tr = $this.parents('tr').next('tr.payment-method-settings-row');
            if ($next_tr.length > 0) {
              var $button = $this.siblings('.edit-payment-method-settings');
              $next_tr.remove();
              $button.click();
            }
            $this.val('Autofilled').addClass('success');
            setTimeout(function() {
              $this.val('LP Autofill').removeClass('success');
            }, 3000);
          }
          $this.prop('disabled', false).removeClass('progress-disabled');
        });
        return false;
      });

      // When an autoconfig button is clicked, make the request.
      $autoconfig = $('.auto-config-settings');
      $autoconfig.on('click', function() {
        var $this = $(this);
        $this.prop('disabled', true).addClass('progress-disabled');
        $.get($this.data('url'), function(response) {
          if (response.length > 0) {
            response = $.parseJSON(response);
            alert(response.message);
            console.error(response.message);
            $this.val('Error').addClass('warning');
            setTimeout(function() {
              $this.val('Dev Config').removeClass('warning');
            }, 3000);
          }
          else {
            var $next_tr = $this.parents('tr').next('tr.payment-method-settings-row');
            if ($next_tr.length > 0) {
              var $button = $this.siblings('.edit-payment-method-settings');
              $next_tr.remove();
              $button.click();
            }
            $this.val('Autoconfigged').addClass('success');
            setTimeout(function() {
              $this.val('Dev Config').removeClass('success');
            }, 3000);
          }
          $this.prop('disabled', false).removeClass('progress-disabled');
        });
        return false;
      });
    }
  }
})(jQuery);
