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

  // @todo
  // $form['payment_gateways']['autoconfigure_all'] = array(
  //   '#type' => 'button',
  //   '#value' => t('Dev config all gateways'),
  // );
  // $form['payment_gateways']['autofill_all'] = array(
  //   '#type' => 'button',
  //   '#value' => t('LP autofill all gateways'),
  // );

  $payment_methods = commerce_payment_methods();

  $options = array('show plugin' => FALSE);
  $conditions = array('event' => 'commerce_payment_methods', 'plugin' => 'reaction rule', 'active' => TRUE);
  $enabled_payment_methods = entity_load('rules_config', FALSE, $conditions);
  $enabled_payment_methods = array_map(function($el) {
    return $el->name;
  }, $enabled_payment_methods);

  $conditions['active'] = FALSE;
  $disabled_payment_methods = entity_load('rules_config', FALSE, $conditions);
  $disabled_payment_methods = array_map(function($el) {
    return $el->name;
  }, $disabled_payment_methods);

  $autofillable_gateways = _springboard_developer_lastpass_supported_gateways();
  $autoconfigurable_gateways = _springboard_developer_autoconfig_supported_gateways();

  $tablerows = array();
  foreach ($payment_methods as $payment_method_name => $payment_method) {
    $rule_name = 'commerce_payment_' . $payment_method['method_id'];
    $instance_id = $payment_method['method_id'] . '|' . $rule_name;
    $instance = commerce_payment_method_instance_load($instance_id);

    $action = 'disable';
    if (in_array($rule_name, $disabled_payment_methods)) {
      $action = 'enable';
    }

    $toggle = ajax_pre_render_element(array(
      '#type' => 'button',
      '#value' => ucfirst(t($action)),
      '#id' => $action . '-' . $payment_method_name,
      '#name' => $action . '-' . $payment_method_name,
      '#prefix' => '<span id="' . $action . "-" . $payment_method_name . '-wrapper">',
      '#suffix' => '</span>',
      '#attributes' => array(
        'class' => array($action . '-payment-method'),
      ),
      '#ajax' => array(
        'path' => 'admin/springboard/springboard-developer/toggle-payment-method/ajax/' . $payment_method_name . '/' . $rule_name . '/' . $action,
        'progress' => array(
          'message' => NULL,
        ),
      ),
    ));

    // If the payment method is being used by any donation forms, disable the
    // disable button.
    if ($action == 'disable' && _springboard_developer_is_payment_method_in_use($instance_id) > 0) {
      $toggle['#attributes']['disabled'] = TRUE;
      $toggle['#attributes']['title'] = t('Cannot disable while donation forms are actively using this payment method.');
    }

    if ($action == 'disable') {
      $toggle['#attributes']['class'] = array('warning');
    }
    else {
      $toggle['#attributes']['class'] = array('success');
    }

    $settings = array(
      '#type' => 'button',
      '#value' => t('Settings'),
      '#name' => 'edit-payment-method-settings-' . $payment_method_name,
      '#attributes' => array(
        'class' => array('edit-payment-method-settings'),
        'data-payment-method' => $payment_method_name,
        'data-url' => '/springboard/springboard-developer/payment-method-settings/nojs/' . $payment_method_name . '/' . $rule_name . '/' . $instance_id,
      ),
    );

    $autofill = '';
    if (!empty($autofillable_gateways[$payment_method_name])) {
      $autofill = array(
        '#type' => 'button',
        '#value' => t('LP Autofill'),
        '#attributes' => array(
          'class' => array('lp-required', 'lp-autofill-settings'),
          'data-url' => '/springboard/springboard-developer/payment-method-settings/nojs/' . $payment_method_name . '/' . $rule_name . '/' . $instance_id . '/autofill',
        ),
      );
    }

    $autoconfig = '';
    if (!empty($autoconfigurable_gateways[$payment_method_name])) {
      $autoconfig = array(
        '#type' => 'button',
        '#value' => t('Dev Config'),
        '#attributes' => array(
          'class' => array('auto-config-settings'),
          'data-url' => '/springboard/springboard-developer/payment-method-settings/nojs/' . $payment_method_name . '/' . $rule_name . '/' . $instance_id . '/autoconfig',
        ),
      );
    }

    $tablerows[$payment_method_name] = array(
      'id' => 'payment-method-' . $payment_method_name . '-wrapper',
      'data' => array(
        t('!payment_method_name: @payment_method_description', array(
          '!payment_method_name' => '<strong>' . $payment_method['short_title'] . '</strong>',
          '@payment_method_description' => $payment_method['description'],
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

  if ($callback = commerce_payment_method_callback($payment_method, 'settings_form')) {
    $method_settings = !empty($instance['settings']) && is_array($instance['settings']) ? $instance['settings'] : array();
    $form['parameter']['payment_method']['settings'] = $callback($method_settings);
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
 * @param string $payment_method_name
 *   The name of the payment method.
 * @param string $rule_name
 *   The name of the rule.
 * @param string $instance_id
 *   The instance id.
 * @param string $action
 *   The action to take, either 'get', 'submit', 'autofill', or 'autoconfig'.
 *   Defaults to 'get'.
 */
function _springboard_developer_admin_settings_payment_method_settings($payment_method_name, $rule_name, $instance_id, $action = 'get') {
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
      _springboard_developer_lastpass_autoconfig($payment_method_name, $rule, $form, $form_state);
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
      $settings_row = '<tr class="payment-method-settings-row"><td class="first">' . render($form) . '</td><td class="last">' . render($actions) . '</td></tr>';

      print $settings_row;
      break;
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
    'callback' => function($obj) {
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
    'callback' => function($obj) {
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
    'callback' => function($obj) {
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
    'callback' => function($obj) {
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
    }
  );
  $gateways['commerce_litle_echeck'] = $gateways['commerce_litle_cc'];

  $gateways['paypal_wps'] = array(
    'search' => '1778982936',
    'fields' => array('id'),
    'callback' => function($obj) {
      return array(
        'business' => $obj->username,
      );
    },
  );

  $gateways['commerce_psi_credit'] = array(
    'search' => '4011923216866530037',
    'fields' => array('id'),
    'callback' => function($obj) {
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
    'callback' => function($obj) {
      preg_match('/^Merchant Id Number\:\s*([0-9]+)$/im', $obj->note, $merchant_id);
      preg_match('/^Merchant key\:\s*([a-z0-9]+)$/im', $obj->note, $merchant_key);
      if (!empty($merchant_id[1]) && !empty($merchant_key[1])) {
        return array(
          'commerce_sage_payments_settings_merchant_id' => $merchant_id[1],
          'commerce_sage_payments_settings_merchant_key' => $merchant_key[1],
          'commerce_sage_payments_settings_application_id' => $merchant_id[1],
        );
      }
    }
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
function _springboard_developer_lastpass_autofill($payment_method_name, $rule, $form, $form_state) {
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
function _springboard_developer_lastpass_autoconfig($payment_method_name, $rule, $form, $form_state) {
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
function _springboard_developer_admin_settings_toggle_payment_method($request_type, $payment_method_name, $rule_name, $action) {
  if ($request_type == 'ajax') {
    $conditions = array(
      'event' => 'commerce_payment_methods',
      'plugin' => 'reaction rule',
      'name' => $rule_name,
    );
    $payment_methods = entity_load('rules_config', FALSE, $conditions);
    $payment_method = reset($payment_methods);

    module_load_include('inc', 'rules', 'ui/ui.forms');
    rules_ui_confirm_operation_apply($action, $payment_method);

    $commands = array();
    $next_action = $action == 'enable' ? 'disable' : 'enable';
    $new_button = ajax_pre_render_element(array(
      '#type' => 'button',
      '#value' => ucfirst(t($next_action)),
      '#id' => $next_action . '-' . $payment_method_name,
      '#name' => $next_action . '-' . $payment_method_name,
      '#prefix' => '<span id="' . $next_action . "-" . $payment_method_name . '-wrapper">',
      '#suffix' => '</span>',
      '#ajax' => array(
        'path' => 'admin/springboard/springboard-developer/toggle-payment-method/ajax/' . $payment_method_name . '/' . $rule_name . '/' . $next_action,
        'progress' => array(
          'message' => NULL,
        ),
      ),
    ));

    if ($next_action == 'disable') {
      $new_button['#attributes']['class'] = array('warning');
    }
    else {
      $new_button['#attributes']['class'] = array('success');
    }

    $new_button = drupal_render($new_button);
    $commands[] = ajax_command_replace('#' . $action . '-' . $payment_method_name, $new_button);

    return array(
      '#type' => 'ajax',
      '#commands' => $commands,
    );
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
function _springboard_developer_is_payment_method_in_use($instance_id) {
  return db_query('SELECT COUNT(did) FROM {fundraiser_donation} WHERE gateway = :instance_id', array(
    ':instance_id' => $instance_id,
  ))->fetchField();
}
