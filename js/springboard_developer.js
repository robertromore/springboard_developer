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
          $select_actions.toggleClass('disabled', !($checkboxes.filter('input:checked').length > 0));
        });

        var $checkboxes = $('table.springboard-developer-edit-payment-gateways td.first input[type="checkbox"]');
        $('table.springboard-developer-edit-payment-gateways th.first input[type="checkbox"]').on('click', function() {
          $checkboxes.prop('checked', !($checkboxes.length == $checkboxes.filter('input:checked').length));
        });

        var $parent_tr;
        $select_actions.click(function() {
          var $this = $(this);
          $checkboxes.filter(':checked').each(function(index, value) {
            var $value = $(value);
            $parent_tr = $value.parents('tr');
            if ($this.hasClass('disable')) {
              $parent_tr.find('input[id^="disable-"]').trigger('mousedown');
            }
            else if ($this.hasClass('enable')) {
              $parent_tr.find('input[id^="enable-"]').trigger('mousedown');
            }
            else if ($this.hasClass('autofill')) {
              $parent_tr.find('input.lp-autofill-settings').trigger('click');
            }
            else if ($this.hasClass('autoconfig')) {
              $parent_tr.find('input.auto-config-settings').trigger('click');
            }
          });
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
      $lp_autoconfig = $('.auto-config-settings');
      $lp_autoconfig.on('click', function() {
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
            $this.val('Autoconfiged').addClass('success');
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
