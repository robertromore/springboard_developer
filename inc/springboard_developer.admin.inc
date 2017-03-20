<?php
/**
 * @file
 * Administrative functions for Springboard Developer.
 */

/**
 * Springboard Developer admin settings form.
 */
function springboard_developer_admin_settings() {
  $form = array();

  $form['springboard_reset'] = ajax_pre_render_element(array(
    '#type' => 'button',
    '#value' => t('Reset Springboard Submissions'),
    '#name' => 'springboard_reset_button',
    '#id' => 'springboard_reset_button',
    '#prefix' => t('Click this button to remove any Springboard submissions, mostly dealing with donations (including webform data). It will also remove all users except the admin user. This is useful if you want to return to a pristine initial state to restart testing.') . '<br/>' . '<div id="springboard-reset-wrapper">',
    '#suffix' => '<span id="reset-button-cleared">All data has been cleared!</span></div>',
    '#attributes' => array(
      'class' => array('warning'),
      'data-url' => '/admin/springboard/springboard-developer/reset-data',
    ),
    '#submit' => array('_springboard_developer_admin_settings_reset_springboard_submissions'),
  ));

  // Integrate with LastPass if the lastpass-php library is available.
  if (_springboard_developer_load_libraries() !== FALSE) {
    $lastpass = new LastPass();
    $form['lastpass'] = array(
      '#type' => 'fieldset',
      '#title' => t('LastPass Integration'),
      '#description' => t('Configure things more quickly by using credentials stored in LastPass.'),
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );
    $form['lastpass']['error'] = array(
      '#type' => 'markup',
      '#markup' => '<span class="lp-login-error lp-failure"></span>',
    );

    $session = $lastpass->getSession();
    $username = '';
    if (!empty($session['lpusername'])) {
      $username = $session['lpusername'];
    }
    $form['lastpass']['message'] = array(
      '#type' => 'markup',
      '#markup' => '<span class="lp-success lp-logged-in">' . t('You are currently logged in as !user.', array('!user' => '<span class="lp-username">' . $username . '</span>')) . '</span> ',
    );
    $form['lastpass']['logout'] = array(
      '#type' => 'button',
      '#value' => t('Log Out'),
      '#name' => 'lastpass_logout_button',
      '#attributes' => array(
        'class' => array('warning', 'lp-logged-in'),
        'data-url' => '/springboard/springboard-developer/lastpass-logout',
      ),
    );

    $form['lastpass']['username'] = array(
      '#type' => 'textfield',
      '#title' => t('LastPass Username'),
      '#prefix' => '<div class="lp-login-field lp-logged-out">',
    );
    $form['lastpass']['password'] = array(
      '#type' => 'password',
      '#title' => t('LastPass password'),
      '#suffix' => '</div>',
    );
    $form['lastpass']['login'] = array(
      '#type' => 'button',
      '#value' => t('Login'),
      '#name' => 'lastpass_login_button',
      '#attributes' => array(
        'class' => array('lp-logged-out'),
        'data-url' => '/springboard/springboard-developer/lastpass-login',
      ),
    );
  }

  $form['payment_gateways'] = array(
    '#type' => 'fieldset',
    '#title' => t('Payment Gateways'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );

  $form['payment_gateways']['select_actions'] = array(
    '#prefix' => '<div class="select-actions select"><span>' . t('With selected') . '</span>',
    '#suffix' => '</div>',
  );
  $form['payment_gateways']['select_actions']['disable'] = array(
    '#type' => 'button',
    '#value' => t('Disable'),
    '#attributes' => array(
      'class' => array('disabled', 'disable'),
      'data-url' => '/springboard/springboard-developer/toggle-payment-method/nojs',
    ),
  );
  $form['payment_gateways']['select_actions']['enable'] = array(
    '#type' => 'button',
    '#value' => t('Enable'),
    '#attributes' => array(
      'class' => array('disabled', 'enable'),
      'data-url' => '/springboard/springboard-developer/toggle-payment-method/nojs',
    ),
  );
  $form['payment_gateways']['select_actions']['autofill'] = array(
    '#type' => 'button',
    '#value' => t('Autofill'),
    '#attributes' => array(
      'class' => array('disabled', 'autofill', 'lp-required'),
      'data-url' => '/springboard/springboard-developer/payment-method-settings/nojs/autofill',
    ),
  );
  $form['payment_gateways']['select_actions']['autoconfig'] = array(
    '#type' => 'button',
    '#value' => t('Autoconfig'),
    '#attributes' => array(
      'class' => array('disabled', 'autoconfig'),
      'data-url' => '/springboard/springboard-developer/payment-method-settings/nojs/autoconfig',
    ),
  );

  $form['payment_gateways']['generate_actions'] = array(
    '#prefix' => '<div class="select-actions generate"><span>Generate</span>',
    '#suffix' => '</div>',
  );
  $form['payment_gateways']['generate_actions']['generate_selected'] = array(
    '#type' => 'button',
    '#value' => t('For selected'),
    '#attributes' => array(
      'class' => array('disabled', 'generate-selected'),
      'data-url' => '/springboard/springboard-developer/create-donation-forms/nojs',
    ),
  );

  $all_payment_methods = commerce_payment_methods();
  $payment_methods = new stdClass();
  foreach ($all_payment_methods as $payment_method) {
    $payment_methods->payment_methods[$payment_method['method_id'] . '|commerce_payment_' . $payment_method['method_id']] = $payment_method;
  }
  $payment_methods = fundraiser_commerce_fundraiser_gateway_info($payment_methods);
  $weights = array('credit', 'bank account', 'paypal', 'applepay');
  $weights = array_flip($weights);
  uasort($payment_methods, function ($a, $b) use ($weights) {
    $a_payment_method = reset($a['payment_method']);
    $b_payment_method = reset($b['payment_method']);
    return $weights[$a_payment_method] - $weights[$b_payment_method];
  });

  $options = array('show plugin' => FALSE);
  $conditions = array(
    'event' => 'commerce_payment_methods',
    'plugin' => 'reaction rule',
    'active' => TRUE,
  );
  $enabled_payment_methods = entity_load('rules_config', FALSE, $conditions);
  $enabled_payment_methods = array_map(function ($el) {
    return $el->name;
  }, $enabled_payment_methods);

  $conditions['active'] = FALSE;
  $disabled_payment_methods = entity_load('rules_config', FALSE, $conditions);
  $disabled_payment_methods = array_map(function ($el) {
    return $el->name;
  }, $disabled_payment_methods);

  $autofillable_gateways = _springboard_developer_lastpass_supported_gateways();
  $autoconfigurable_gateways = _springboard_developer_autoconfig_supported_gateways();

  $tablerows = array();
  $last_payment_method_type = reset($payment_methods);
  $last_payment_method_type = reset($last_payment_method_type['payment_method']);
  $tablerows[] = array(
    'data' => array(
      array(),
      array(),
      '<strong>' . t(ucfirst($last_payment_method_type)) . '</strong>',
      array(),
    ),
  );
  foreach ($payment_methods as $payment_method_name => $payment_method) {
    $payment_method_type = reset($payment_method['payment_method']);
    if ($payment_method_type != $last_payment_method_type) {
      $tablerows[] = array(
        'data' => array(
          array(),
          array(),
          '<strong>' . t(ucfirst($payment_method_type)) . '</strong>',
          array(),
        ),
      );
      $last_payment_method_type = $payment_method_type;
    }
    list($base_name, $rule_name) = explode('|', $payment_method_name);

    $action = 'disable';
    if (in_array($rule_name, $disabled_payment_methods)) {
      $action = 'enable';
    }

    $toggle = array(
      '#type' => 'button',
      '#value' => ucfirst(t($action)),
      '#id' => $action . '-' . $base_name,
      '#attributes' => array(
        'class' => array($action . '-payment-method', $action),
        'data-url' => '/springboard/springboard-developer/toggle-payment-method/nojs/' . $payment_method_name . '/' . $action,
      ),
    );

    // If the payment method is being used by any donation forms, disable the
    // disable button.
    $method = array(
      'status' => 1,
      'method' => reset($payment_method['payment_method']),
      'id' => $payment_method['id'],
    );
    $serialized_method = serialize($method);
    if (preg_match('/a:3:\{([^\}]*)\}/', $serialized_method, $matches)) {
      $serialized_method = $matches[1];
    }
    if ($action == 'disable' && _springboard_developer_is_payment_method_in_use($serialized_method)) {
      $toggle['#attributes']['disabled'] = TRUE;
      $toggle['#attributes']['title'] = t('Cannot disable while donation forms are actively using this payment method.');
    }

    if ($action == 'disable') {
      $toggle['#attributes']['class'][] = 'warning';
    }
    else {
      $toggle['#attributes']['class'][] = 'success';
    }

    $settings = array(
      '#type' => 'button',
      '#value' => t('Settings'),
      '#name' => 'edit-payment-method-settings-' . $base_name,
      '#attributes' => array(
        'class' => array('edit-payment-method-settings'),
        'data-payment-method' => $base_name,
        'data-url' => '/springboard/springboard-developer/payment-method-settings/nojs/' . $payment_method_name,
      ),
    );

    $autofill = '';
    if (!empty($autofillable_gateways[$base_name])) {
      $autofill = array(
        '#type' => 'button',
        '#value' => t('LP Autofill'),
        '#attributes' => array(
          'class' => array('lp-required', 'lp-autofill-settings'),
          'data-url' => '/springboard/springboard-developer/payment-method-settings/nojs/' . $payment_method_name . '/autofill',
        ),
      );
    }

    $autoconfig = '';
    if (!empty($autoconfigurable_gateways[$base_name])) {
      $autoconfig = array(
        '#type' => 'button',
        '#value' => t('Dev Config'),
        '#attributes' => array(
          'class' => array('auto-config-settings'),
          'data-url' => '/springboard/springboard-developer/payment-method-settings/nojs/' . $payment_method_name . '/autoconfig',
        ),
      );
    }

    $tablerows[$base_name] = array(
      'id' => 'payment-method-' . $base_name . '-wrapper',
      'data-payment-method-id' => $payment_method_name,
      'data' => array(
        array(
          'data' => array(
            '#type' => 'checkbox',
            '#default_value' => FALSE,
            '#name' => 'toggle-row',
          ),
        ),
        array(
          'data' => array(
            '#type' => 'radio',
            '#default_value' => FALSE,
            '#name' => 'payment-method-type-' . str_replace(' ', '-', $payment_method_type),
            '#attributes' => array(
              'data-payment-method-type' => $payment_method_type,
              'data-payment-method-id' => $payment_method_name,
            ),
          ),
        ),
        t('!payment_method_name: @payment_method_description', array(
          '!payment_method_name' => '<strong>' . $payment_method['gateway_details']['short_title'] . '</strong>',
          '@payment_method_description' => $payment_method['gateway_details']['description'],
        )),
        array(
          'data' => array($toggle, $settings, $autofill, $autoconfig),
        ),
      ),
    );
  }

  $form['payment_gateways']['payment_gateways'] = array(
    '#theme' => 'table',
    '#rows' => $tablerows,
    '#header' => array(
      array(
        'data' => array(
          '#colspan' => 2,
          array(
            '#type' => 'checkbox',
            '#default_value' => FALSE,
            '#name' => 'toggle-row',
          ),
        ),
      ),
      array(
        'data' => array(
          '#colspan' => 2,
        ),
      ),
      t('Name: Description'),
      t('Operations'),
    ),
    '#attributes' => array(
      'class' => array('springboard-developer-edit-payment-gateways'),
    ),
  );

  $form['#attached']['css'] = array(
    drupal_get_path('module', 'springboard_developer') . '/css/springboard_developer.css',
  );
  $form['#attached']['js'] = array(
    drupal_get_path('module', 'springboard_developer') . '/js/springboard_developer.js',
  );

  return $form;
}

/**
 * Utility function to help set some necessary variables.
 *
 * @param string $payment_method_name
 *   The payment method name.
 * @param string $rule_name
 *   The rule name.
 * @param string $instance_id
 *   The instance ID.
 *
 * @return array
 *   An array of the payment method, instance, rule instance, generated form,
 *   and form_state.
 */
function _springboard_developer_load_payment_method_info($payment_method_name, $rule_name, $instance_id) {
  $payment_method = commerce_payment_method_load($payment_method_name);

  $instance_id = urldecode($instance_id);
  $instance = commerce_payment_method_instance_load($instance_id);

  $form['parameter'] = array(
    '#tree' => TRUE,
    'payment_method' => array(
      'method_id' => array(
        '#type' => 'value',
        '#value' => $payment_method_name,
      ),
    ),
  );

  $method_settings = array();
  if ($callback = commerce_payment_method_callback($payment_method, 'settings_form')) {
    $method_settings = !empty($instance['settings']) && is_array($instance['settings']) ? $instance['settings'] : array();
    $form['parameter']['payment_method']['settings'] = $callback($method_settings);
  }

  // If $method_settings is empty but settings were returned from the callback,
  // compute and add them to the $method_settings array.
  if (empty($method_settings) && !empty($form['parameter']['payment_method']['settings'])) {
    foreach ($form['parameter']['payment_method']['settings'] as $form_setting_key => $form_setting) {
      $children = element_children($form_setting);
      if (empty($children)) {
        // This is a leaf.
        $method_settings[$form_setting_key] = $form_setting['#default_value'];
      }
      else {
        // This isn't a leaf, so do same procedure to its children.
        // @todo If a payment method ever has settings which is more than 2
        // dimensions, this will need to be changed to be recursive.
        foreach ($children as $child) {
          $method_settings[$form_setting_key][$child] = $form_setting[$child]['#default_value'];
        }
      }
    }
  }

  module_load_include('inc', 'fundraiser_commerce', '/gateways/' . $payment_method['module']);
  $gateway_info = module_invoke($payment_method['module'], 'fundraiser_commerce_fundraiser_gateway_info');
  if (!empty($gateway_info['allow_recurring']) && !empty($gateway_info['offsite_recurring'])) {
    $form['parameter']['payment_method']['settings']['offsite_recurring'] = array(
      '#type' => 'checkbox',
      '#title' => t('Offsite recurring donations'),
      '#description' => t('Handle recurring donations offsite at the gateway.'),
      '#default_value' => isset($instance['settings']['offsite_recurring']) ? $instance['settings']['offsite_recurring'] : FALSE,
    );
  }

  list($method_id, $rule_name) = explode('|', $instance_id);
  $rule = rules_config_load($rule_name);

  $form_state = array(
    'rebuild' => FALSE,
    'rebuild_info' => array(),
    'redirect' => NULL,
    'temporary' => array(),
    'submitted' => FALSE,
    'executed' => FALSE,
    'programmed' => FALSE,
    'programmed_bypass_access_check' => TRUE,
    'cache' => FALSE,
    'method' => 'get',
    'groups' => array(),
    'buttons' => array(),
    'input' => array(),
    'rules_element' => $rule,
    '_rules_base_path' => 'admin/commerce/config/payment-methods',
    'element_settings' => array(
      'commerce_order:select' => 'commerce-order',
      'payment_method' => $form['parameter']['payment_method'],
    ),
    'parameter_mode' => array(
      'commerce_order' => 'selector',
      'payment_method' => 'input',
    ),
    'storage' => array(
      'old_settings_assoc' => $method_settings,
    ),
    'values' => array(
      'parameter' => array(
        'commerce_order' => array(
          'settings' => array(
            'commerce_order:select' => 'commerce-order',
            'switch_button' => 'Switch to the direct input mode',
          ),
        ),
        'payment_method' => array(
          'settings' => array(
            'payment_method' => array(
              'method_id' => $payment_method_name,
              'settings' => $method_settings,
            ),
          ),
        ),
      ),
    ),
  );

  drupal_prepare_form('springboard_developer_payment_method_settings_' . $payment_method_name, $form, $form_state);

  return array($payment_method, $instance, $rule, $form, $form_state);
}

/**
 * Delegates to appropriate functions based on the action.
 *
 * @param string $instance_ids
 *   A comma-delimited string of instance ids.
 * @param string $action
 *   The action to take, either 'get', 'submit', 'autofill', or 'autoconfig'.
 *   Defaults to 'get'.
 */
function _springboard_developer_admin_settings_payment_method_settings($instance_ids, $action = 'get') {
  $instance_ids = urldecode($instance_ids);
  if (strpos($instance_ids, ',') === FALSE) {
    $instance_ids = array($instance_ids);
  }
  else {
    $instance_ids = explode(',', $instance_ids);
  }
  foreach ($instance_ids as $instance_id) {
    list($payment_method_name, $rule_name) = explode('|', $instance_id);

    list($payment_method, $instance, $rule, $form, $form_state) = _springboard_developer_load_payment_method_info($payment_method_name, $rule_name, $instance_id);

    switch ($action) {
      case 'submit':
        $data = $_POST['parameter']['payment_method']['settings'];
        foreach ($form_state['values']['parameter']['payment_method']['settings']['payment_method']['settings'] as $key => &$setting) {
          if (!isset($data[$key])) {
            if (is_array($setting)) {
              foreach ($setting as &$s) {
                $s = NULL;
              }
            }
            else {
              $setting = NULL;
            }
          }
          else {
            $setting = $data[$key];
          }
        }

        _springboard_developer_save_payment_method_form($rule, $form, $form_state);
        break;

      case 'autofill':
        _springboard_developer_lastpass_autofill($payment_method_name, $rule, $form, $form_state);
        break;

      case 'autoconfig':
        _springboard_developer_autoconfig($payment_method_name, $rule, $form, $form_state);
        break;

      case 'get':
      default:
        $actions = array();
        $actions['save'] = array(
          '#type' => 'button',
          '#value' => t('Save'),
          '#attributes' => array(
            'class' => array('success', 'save-payment-method-settings'),
            'data-payment-method' => $payment_method_name,
            'data-url' => '/admin/springboard/springboard-developer/payment-method-settings/nojs/' . $payment_method_name . '/' . $rule_name . '/' . $instance_id,
          ),
          '#id' => 'save-payment-settings-' . $payment_method_name,
        );
        $actions['cancel'] = array(
          '#type' => 'button',
          '#value' => t('Cancel'),
          '#attributes' => array(
            'class' => array('warning', 'remove-payment-method-setting-row'),
          ),
        );

        $form = form_builder('springboard_developer_payment_method_settings_' . $payment_method_name, $form, $form_state);
        $settings_row = '<tr class="payment-method-settings-row"><td class="first"></td><td>' . render($form) . '</td><td class="last">' . render($actions) . '</td></tr>';

        print $settings_row;
        break;
    }
  }
}

/**
 * Return an array of gateways which support autofilling with LastPass.
 *
 * @return array
 *   An array of supported gateways.
 */
function _springboard_developer_lastpass_supported_gateways() {
  $gateways = array();
  $gateways['authnet_aim'] = array(
    'search' => '5734027324106971530',
    'fields' => array('id'),
    'callback' => function ($obj) {
      // The API and transaction key are stored in the record's note.
      preg_match('/API Login ID: ([a-z0-9]+)/i', $obj->note, $api_key);
      preg_match('/Transaction Key: ([a-z0-9]+)/i', $obj->note, $trans_key);
      return array('login' => $api_key[1], 'tran_key' => $trans_key[1]);
    },
  );
  $gateways['authnet_echeck'] = $gateways['authnet_aim'];

  $gateways['braintree_hostedfields'] = array(
    'search' => '5575660452',
    'fields' => array('id'),
    'callback' => function ($obj) {
      preg_match('/Merchant Account ID: ([a-z]+)/i', $obj->note, $merchant_account_id);
      preg_match('/Merchant ID: ([a-z0-9]+)/i', $obj->note, $account_id);
      preg_match('/Public Key: ([a-z0-9]+)/i', $obj->note, $public_key);
      preg_match('/Private Key: ([a-z0-9]+)/i', $obj->note, $private_key);
      preg_match('/Channel\/Partner ID: ([a-z0-9\_\-]+)/i', $obj->note, $channel_id);
      return array(
        'merchant_id' => array(
          'USD' => $merchant_account_id[1],
        ),
        'sandbox' => array(
          'merchant_id' => $account_id[1],
          'public_key' => $public_key[1],
          'private_key' => $private_key[1],
          'channel' => $channel_id[1],
        ),
      );
    },
  );
  $gateways['braintree_paypal'] = $gateways['braintree_hostedfields'];
  $gateways['braintree_applepay'] = $gateways['braintree_hostedfields'];

  $gateways['cybersource_soap_cc'] = array(
    'search' => '3674961836',
    'fields' => array('id'),
    'callback' => function ($obj) {
      preg_match('/merchant id: ([a-z0-9\_\-]+)/i', $obj->note, $merchant_id);
      preg_match('/transaction key:\s*([a-z0-9\+\_\/\=]+)/i', $obj->note, $transaction_key);
      return array(
        'merchant_id' => $merchant_id[1],
        'transaction_key' => $transaction_key[1],
      );
    },
  );

  $gateways['commerce_litle_cc'] = array(
    'search' => '8227011218062340343',
    'fields' => array('id'),
    'callback' => function ($obj) {
      preg_match('/User to\s*"([a-z]+)"/i', $obj->note, $user);
      preg_match('/Password to\s*"([a-z0-9]+)"/i', $obj->note, $password);
      preg_match('/Merchant Id to\s*"([0-9]+)"/i', $obj->note, $merchant_id);
      preg_match('/URL to\s*"([a-z:\/\.\-]+)"/i', $obj->note, $url);
      preg_match('/Report group to\s*"([a-z\_\-]+)"/i', $obj->note, $report_group);
      return array(
        'commerce_litle_settings_user' => $user[1],
        'commerce_litle_settings_password' => $password[1],
        'commerce_litle_settings_merchantId' => $merchant_id[1],
        'commerce_litle_settings_url' => $url[1],
        'commerce_litle_settings_reportGroup' => $report_group[1],
        'commerce_litle_settings_sandbox' => 1,
      );
    },
  );
  $gateways['commerce_litle_echeck'] = $gateways['commerce_litle_cc'];

  $gateways['paypal_wps'] = array(
    'search' => '1778982936',
    'fields' => array('id'),
    'callback' => function ($obj) {
      return array(
        'business' => $obj->username,
      );
    },
  );

  $gateways['commerce_psi_credit'] = array(
    'search' => '4011923216866530037',
    'fields' => array('id'),
    'callback' => function ($obj) {
      return array(
        'account_username' => $obj->username,
        'account_password' => $obj->password,
      );
    },
  );
  $gateways['commerce_psi_eft'] = $gateways['commerce_psi_credit'];

  $gateways['commerce_sage_payments_cc'] = array(
    'search' => '2669351256',
    'fields' => array('id'),
    'callback' => function ($obj) {
      preg_match('/^Merchant Id Number\:\s*([0-9]+)$/im', $obj->note, $merchant_id);
      preg_match('/^Merchant key\:\s*([a-z0-9]+)$/im', $obj->note, $merchant_key);
      if (!empty($merchant_id[1]) && !empty($merchant_key[1])) {
        return array(
          'commerce_sage_payments_settings_merchant_id' => $merchant_id[1],
          'commerce_sage_payments_settings_merchant_key' => $merchant_key[1],
          'commerce_sage_payments_settings_application_id' => $merchant_id[1],
        );
      }
    },
  );
  $gateways['commerce_sage_payments_eft'] = $gateways['commerce_sage_payments_cc'];

  drupal_alter('springboard_developer_lastpass_supported_gateways', $gateways);
  return $gateways;
}

/**
 * Return an array of gateways which support auto configuration.
 *
 * @return array
 *   An array of supported gateways.
 */
function _springboard_developer_autoconfig_supported_gateways() {
  $gateways = array();
  $default = array(
    'log' => array(
      'request' => 'request',
      'response' => 'response',
    ),
  );
  $gateways['authnet_aim'] = $default + array(
    'txn_mode' => 'developer',
  );
  $gateways['authnet_echeck'] = $default + array(
    'txn_mode' => 'developer',
  );

  $gateways['braintree_hostedfields'] = $default + array(
    'environment' => 'sandbox',
  );
  $gateways['braintree_paypal'] = $gateways['braintree_hostedfields'];
  $gateways['braintree_applepay'] = $gateways['braintree_hostedfields'];

  $gateways['cybersource_soap_cc'] = $default + array(
    'transaction_mode' => 'test',
  );

  $gateways['commerce_litle_cc'] = $default;
  $gateways['commerce_litle_echeck'] = $gateways['commerce_litle_cc'];

  $gateways['paypal_wps'] = array(
    'server' => 'sandbox',
    'payment_action' => 'sale',
    'ipn_logging' => 'notification',
  );

  $gateways['commerce_psi_credit'] = array(
    'txn_mode' => 'test',
    'card_types' => array(
      'visa' => 'visa',
      'mastercard' => 'mastercard',
      'amex' => 'amex',
      'discover' => 'discover',
    ),
  );
  $gateways['commerce_psi_eft'] = $gateways['commerce_psi_credit'];

  $gateways['commerce_sage_payments_cc'] = $default + array(
    'transaction_type' => 'auth_capture',
  );
  $gateways['commerce_sage_payments_eft'] = $gateways['commerce_sage_payments_cc'];

  return $gateways;
}

/**
 * Utility function to autofill a payment method with values from LastPass.
 *
 * @param string $payment_method_name
 *   The payment method name.
 * @param object $rule
 *   The rule instance.
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 */
function _springboard_developer_lastpass_autofill($payment_method_name, $rule, array $form, array $form_state) {
  if (_springboard_developer_load_libraries() === FALSE) {
    $result = array(
      'status' => 'error',
      'message' => t("Error: couldn't load library."),
    );
    print json_encode($result);
    return;
  }

  $lp_gateways = _springboard_developer_lastpass_supported_gateways();
  if (empty($lp_gateways[$payment_method_name])) {
    $result = array(
      'status' => 'error',
      'message' => t('Error: specified gateway not found.'),
    );
    print json_encode($result);
    return;
  }

  $lp = new LastPass();
  if (!$lp->isLoggedIn()) {
    $result = array(
      'status' => 'error',
      'message' => t('Error: not logged in!'),
    );
    print json_encode($result);
    return;
  }

  try {
    $search = $lp->searchAccounts($lp_gateways[$payment_method_name]['search'], $lp_gateways[$payment_method_name]['fields']);
    $search = reset($search);
    $new_settings = $lp_gateways[$payment_method_name]['callback']($search);
    if (!empty($new_settings)) {
      foreach ($new_settings as $nk => $ns) {
        if (array_key_exists($nk, $form_state['values']['parameter']['payment_method']['settings']['payment_method']['settings'])) {
          $form_state['values']['parameter']['payment_method']['settings']['payment_method']['settings'][$nk] = $ns;
        }
      }
      _springboard_developer_save_payment_method_form($rule, $form, $form_state);
      // Don't print anything, JS will know the result was successful.
      print '';
    }
    else {
      $result = array(
        'status' => 'error',
        'message' => t('No new settings.'),
      );
      return json_encode($result);
    }
  }
  catch (Exception $e) {
    $result = array(
      'status' => 'error',
      'message' => t('Error: @error', array('@error' => $e->getMessage())),
    );
    print json_encode($result);
    return;
  }
}

/**
 * Utility function to autoconfig a payment method with developer values.
 *
 * @param string $payment_method_name
 *   The payment method name.
 * @param object $rule
 *   The rule instance.
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 */
function _springboard_developer_autoconfig($payment_method_name, $rule, $form, $form_state) {
  $gateways = _springboard_developer_autoconfig_supported_gateways();
  if (empty($gateways[$payment_method_name])) {
    $result = array(
      'status' => 'error',
      'message' => t('Error: specified gateway not found.'),
    );
    print json_encode($result);
    return;
  }

  $new_settings = $gateways[$payment_method_name];
  foreach ($new_settings as $nk => $ns) {
    if (array_key_exists($nk, $form_state['values']['parameter']['payment_method']['settings']['payment_method']['settings'])) {
      $form_state['values']['parameter']['payment_method']['settings']['payment_method']['settings'][$nk] = $ns;
    }
  }

  _springboard_developer_save_payment_method_form($rule, $form, $form_state);
}

/**
 * Utility function to save new settings to a payment method.
 *
 * @param object $rule
 *   The rule object.
 * @param array $form
 *   The form array.
 * @param array $form_state
 *   The form state array.
 */
function _springboard_developer_save_payment_method_form($rule, $form, $form_state) {
  $rule->form_validate($form, $form_state);
  $rule->form_submit($form, $form_state);

  foreach ($rule->actions() as $action) {
    $action->form_validate($form, $form_state);
    $action->form_submit($form, $form_state);
  }
}

/**
 * Utility function to toggle a payment method on or off.
 */
function _springboard_developer_admin_settings_toggle_payment_method($request_type, $instance_ids, $action) {
  $instance_ids = urldecode($instance_ids);
  if (strpos($instance_ids, ',') === FALSE) {
    $instance_ids = array($instance_ids);
  }
  else {
    $instance_ids = explode(',', $instance_ids);
  }
  foreach ($instance_ids as $instance_id) {
    list($payment_method_name, $rule_name) = explode('|', $instance_id);

    $conditions = array(
      'event' => 'commerce_payment_methods',
      'plugin' => 'reaction rule',
      'name' => $rule_name,
    );
    $payment_methods = entity_load('rules_config', FALSE, $conditions);
    $payment_method = reset($payment_methods);

    module_load_include('inc', 'rules', 'ui/ui.forms');
    rules_ui_confirm_operation_apply($action, $payment_method);

    $next_action = $action == 'enable' ? 'disable' : 'enable';
    $new_button = ajax_pre_render_element(array(
      '#type' => 'button',
      '#value' => ucfirst(t($next_action)),
      '#id' => $next_action . '-' . $payment_method_name,
      '#attributes' => array(
        'class' => array($next_action . '-payment-method', $next_action),
        'data-url' => '/springboard/springboard-developer/toggle-payment-method/nojs/' . $instance_id . '/' . $next_action,
      ),
    ));

    if ($next_action == 'disable') {
      $new_button['#attributes']['class'][] = 'warning';
    }
    else {
      $new_button['#attributes']['class'][] = 'success';
    }

    print render($new_button) . ',';
  }
}

/**
 * AJAX callback function for Springboard Developer admin settings form.
 *
 * Attempts to log users in with LastPass.
 *
 * @see springboard_developer_admin_settings()
 */
function _springboard_developer_admin_settings_lastpass_login() {
  $library_name = 'lastpass-php';
  $result = array();
  if (_springboard_developer_load_libraries() !== FALSE) {
    $username = $_POST['username'];
    $password = $_POST['password'];
    try {
      $lastpass = new LastPass($username, $password);
      if ($lastpass->login()) {
        $result['status'] = 'ok';
        $result['username'] = $username;
      }
      else {
        $result['status'] = 'error';
        $result['message'] = t('Unable to log you in with the credentials you provided.');
      }
    }
    catch (Exception $e) {
      $result['status'] = 'error';
      $result['message'] = t('Error: @error', array('@error' => $e->getMessage()));
    }
  }
  print json_encode($result);
}

/**
 * AJAX callback function for Springboard Developer admin settings form.
 *
 * Attempts to log users out of LastPass.
 *
 * @see springboard_developer_admin_settings()
 */
function _springboard_developer_admin_settings_lastpass_logout($form, $form_state) {
  $library_name = 'lastpass-php';
  if (_springboard_developer_load_libraries()) {
    $lastpass = new LastPass();
    $lastpass->logout();
    $result = array(
      'status' => 'ok',
    );
    print json_encode($result);
  }
}

/**
 * Helper function to retrieve number of donations using a specified gateway.
 */
function _springboard_developer_is_payment_method_in_use($serialized_data) {
  return db_query('SELECT COUNT(nid) FROM {fundraiser} WHERE gateways LIKE :serialized', array(
    ':serialized' => '%' . db_like($serialized_data) . '%',
  ))->fetchField();
}

const LOREMS = array(
  'Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.',
  'Sed ut perspiciatis unde omnis iste natus error sit voluptatem accusantium doloremque laudantium, totam rem aperiam, eaque ipsa quae ab illo inventore veritatis et quasi architecto beatae vitae dicta sunt explicabo. Nemo enim ipsam voluptatem quia voluptas sit aspernatur aut odit aut fugit, sed quia consequuntur magni dolores eos qui ratione voluptatem sequi nesciunt. Neque porro quisquam est, qui dolorem ipsum quia dolor sit amet, consectetur, adipisci velit, sed quia non numquam eius modi tempora incidunt ut labore et dolore magnam aliquam quaerat voluptatem. Ut enim ad minima veniam, quis nostrum exercitationem ullam corporis suscipit laboriosam, nisi ut aliquid ex ea commodi consequatur? Quis autem vel eum iure reprehenderit qui in ea voluptate velit esse quam nihil molestiae consequatur, vel illum qui dolorem eum fugiat quo voluptas nulla pariatur?',
  'But I must explain to you how all this mistaken idea of denouncing pleasure and praising pain was born and I will give you a complete account of the system, and expound the actual teachings of the great explorer of the truth, the master-builder of human happiness. No one rejects, dislikes, or avoids pleasure itself, because it is pleasure, but because those who do not know how to pursue pleasure rationally encounter consequences that are extremely painful. Nor again is there anyone who loves or pursues or desires to obtain pain of itself, because it is pain, but because occasionally circumstances occur in which toil and pain can procure him some great pleasure. To take a trivial example, which of us ever undertakes laborious physical exercise, except to obtain some advantage from it? But who has any right to find fault with a man who chooses to enjoy a pleasure that has no annoying consequences, or one who avoids a pain that produces no resultant pleasure?',
  'At vero eos et accusamus et iusto odio dignissimos ducimus qui blanditiis praesentium voluptatum deleniti atque corrupti quos dolores et quas molestias excepturi sint occaecati cupiditate non provident, similique sunt in culpa qui officia deserunt mollitia animi, id est laborum et dolorum fuga. Et harum quidem rerum facilis est et expedita distinctio. Nam libero tempore, cum soluta nobis est eligendi optio cumque nihil impedit quo minus id quod maxime placeat facere possimus, omnis voluptas assumenda est, omnis dolor repellendus. Temporibus autem quibusdam et aut officiis debitis aut rerum necessitatibus saepe eveniet ut et voluptates repudiandae sint et molestiae non recusandae. Itaque earum rerum hic tenetur a sapiente delectus, ut aut reiciendis voluptatibus maiores alias consequatur aut perferendis doloribus asperiores repellat.',
);

/**
 * Menu callback to generate donation forms.
 */
function _springboard_developer_admin_settings_generage_donation_forms() {
  $post = $_POST;
  $gateway_data = array();
  foreach ($post as $k => $p) {
    if (preg_match('/([a-z]+)\_([0-9]+)/i', $k, $matches) !== FALSE) {
      $gateway_data[$matches[2]][$matches[1]] = $p;
    }
  }

  $instance_labels = array(
    'credit' => t('Credit'),
    'bank account' => t('Bank'),
    'paypal' => t('Paypal'),
    'applepay' => t('ApplePay'),
  );
  $instance_initials = array(
    'authnet_aim|commerce_payment_authnet_aim' => 'AUTHNETCC',
    'commerce_payment_example|commerce_payment_commerce_payment_example' => 'EXCC',
    'commerce_psi_credit|commerce_payment_commerce_psi_credit' => 'PSICC',
    'commerce_sage_payments_cc|commerce_payment_commerce_sage_payments_cc' => 'SAGECC',
    'cybersource_soap_cc|commerce_payment_cybersource_soap_cc' => 'CYBERCC',
    'commerce_litle_cc|commerce_payment_commerce_litle_cc' => 'LITLECC',
    'braintree_hostedfields|commerce_payment_braintree_hostedfields' => 'BTCC',
    'authnet_echeck|commerce_payment_authnet_echeck' => 'AUTHNETEFT',
    'commerce_sage_payments_eft|commerce_payment_commerce_sage_payments_eft' => 'SAGEEFT',
    'commerce_litle_echeck|commerce_payment_commerce_litle_echeck' => 'LITLEEFT',
    'commerce_psi_eft|commerce_payment_commerce_psi_eft' => 'PSIEFT',
    'paypal_wps|commerce_payment_paypal_wps' => 'PAYPALWPS',
    'braintree_paypal|commerce_payment_braintree_paypal' => 'BTPAYPAL',
    'braintree_applepay|commerce_payment_braintree_applepay' => 'BTAPPLEPAY',
  );
  $instances_title = array();

  foreach ($gateway_data as $k => $gd) {
    $gateway_data[$gd['type']] = array(
      'status' => 1,
      'method' => $gd['type'],
      'id' => $gd['id'],
      'weight' => 0,
      'label' => $instance_labels[$gd['type']],
      'default' => $gd['type'] == 'credit',
    );

    $instances_title[] = $instance_initials[$gd['id']];
    unset($gateway_data[$k]);
  }

  $instances_title = implode('/', $instances_title);

  global $user;
  $node = new stdClass();
  $node->type = 'donation_form';
  $node->uid = $user->uid;
  $node->status = 1;
  $node->gateways = $gateway_data;
  $node->body[LANGUAGE_NONE][0]['value'] = LOREMS[rand(0, 3)];
  $node->title = $node->field_fundraiser_internal_name[LANGUAGE_NONE][0]['value'] = 'Generated Donation Form - ' . $instances_title;
  $node->language = LANGUAGE_NONE;
  $node->recurring_setting = 'user_choice';
  $node->recurring_dual_ask_amounts = 0;
  $node->recurring_donation_type_component = checkbox;
  $node->recurring_default_donation_type = NO_RECURR;
  $node->recurring_donation_help_text = 'Selecting "Monthly" will cause your credit card to be charged once per month until its expiration date.';
  $node->send_new_user_mail = 1;
  $node->is_webform_user = TRUE;
  $node->field_fundraiser_currency[LANGUAGE_NONE][0]['value'] = 'USD';
  $node->donation_amounts = $node->recurring_donation_amounts = array(
    array(
      'default_amount' => 0,
      'amount' => 10,
      'label' => '$10',
      'weight' => 0,
    ),
    array(
      'default_amount' => 0,
      'amount' => 20,
      'label' => '$20',
      'weight' => 1,
    ),
    array(
      'default_amount' => 0,
      'amount' => 30,
      'label' => '$30',
      'weight' => 2,
    ),
    array(
      'default_amount' => 0,
      'amount' => 40,
      'label' => '$40',
      'weight' => 3,
    ),
  );
  $node->show_other_amount = $node->recurring_show_other_amount = 1;
  $node->minimum_donation_amount = $node->recurring_minimum_donation_amount = 10;
  $node->recurring_default_amount = NULL;
  $node->recurring_donation_type_options = array('NO_RECURR|One-time', 'recurs|Monthly');

  node_object_prepare($node);
  node_save($node);
}
