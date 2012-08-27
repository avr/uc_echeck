<?php

/**
 * @file
 * eCheck administration menu items.
 */

/**
 * Displays the credit card terminal page.
 */
function uc_echeck_terminal($order) {
  $output = l(t('Return to order view screen.'), 'admin/store/orders/'. $order->order_id);

  $output .= '<p>'. t('Use this terminal to process eCheck payments through your default gateway.') .'</p>';

  if (!variable_get('uc_echeck_debug', FALSE)) {
    $output .= '<p>'. t('Be warned that eCheck data will automatically be converted to the last 4 digits of the account number once a transaction has occurred.  As such, subsequent charges after a partial payment will not have any stored eCheck information to use.') .'</p>';
  }

  $context = array(
    'revision' => 'formatted-original',
    'type' => 'order_total',
    'subject' => array(
      'order' => $order,
    ),
  );
  $output .= '<div><strong>'. t('Order total: @total', array('@total' => uc_price($order->order_total, $context))) .'</strong></div>'
            .'<div><strong>'. t('Balance: @balance', array('@balance' => uc_price(uc_payment_balance($order), $context))) .'</strong></div>';

  $output .= drupal_get_form('uc_echeck_terminal_form', $order);

  return $output;
}

/**
 * Displays the eCheck terminal form for administrators.
 *
 * @see uc_echeck_terminal_form_validate()
 * @see uc_echeck_terminal_form_submit()
 */

function uc_echeck_terminal_form($form_state, $order) {
   // Get the transaction types available to our default gateway.
  $types = uc_echeck_gateway_txn_types(uc_credit_default_gateway());

  // Put the order ID in the form.
  $form['order_id'] = array(
    '#type' => 'hidden',
    '#value' => $order->order_id,
  );

  $balance = uc_payment_balance($order);

  $context = array(
    'revision' => 'formatted-original',
    'type' => 'amount',
  );
  $options = array(
    'sign' => FALSE,
    'thou' => FALSE,
    'dec' => '.',
  );

  // Let the administrator set the amount of payment.
  $form['amount'] = array(
    '#type' => 'textfield',
    '#title' => t('Payment Amount'),
    '#default_value' => $balance > 0 ? uc_price($balance, $context, $options) : 0,
    '#size' => 10,
    '#disabled' => $lock_amount,
    '#field_prefix' => variable_get('uc_sign_after_amount', FALSE) ? '' : variable_get('uc_currency_sign', '$'),
    '#field_suffix' => variable_get('uc_sign_after_amount', FALSE) ? variable_get('uc_currency_sign', '$') : '',
  );

  $form['specify_echeck'] = array(
    '#type' => 'fieldset',
    '#title' => t('Payment details'),
    '#tree' => TRUE,
  );

  $form['specify_echeck']['echeck_data'] = array(
    '#tree' => TRUE,
  );

  $form['specify_echeck']['echeck_data'] += uc_payment_method_echeck_form($form_state, $order);
  unset($form['specify_echeck']['echeck_data']['echeck_policy']);

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Process payment'),
    '#suffix' => l(t('Cancel'), 'admin/store/orders/'. $order->order_id),
  );

  return $form;
}

/**
 * Validation handler for echeck terminal form.
 *
 * @see uc_echeck_terminal_form()
 * @see uc_echeck_terminal_form_submit()
 */
function uc_echeck_terminal_form_validate($form, &$form_state) {
  if (!is_numeric($form_state['values']['amount']) || $form_state['values']['amount'] <= 0) {
    form_set_error('amount', t('You must enter a positive number for the payment amount.'));
  }

  if (uc_order_load($form_state['values']['order_id']) === FALSE) {
    form_set_error('', t('Invalid order ID.  Unable to process payment.'));
  }
}

/**
 * Submit handler for eCheck terminal form.
 *
 * @see uc_echeck_terminal_form()
 * @see uc_echeck_terminal_form_validate()
 */
function uc_echeck_terminal_form_submit($form, &$form_state) {
  // Load the order.
  $order = uc_order_load($form_state['values']['order_id']);

  // Get the data from the form and replace masked data from the order.
  $echeck_data = $form_state['values']['echeck_data'];

  if (strpos($echeck_data['echeck_bank_aba_code'], t('(Last 4) ')) === 0) {
    $echeck_data['echeck_bank_aba_code'] = $order->payment_details['echeck_bank_aba_code'];
  }

  if (strpos($echeck_data['echeck_bank_acct_num'], t('(Last 4) ')) === 0) {
    $echeck_data['echeck_bank_acct_num'] = $order->payment_details['echeck_bank_acct_num'];
  }
  
  // Cache the values for use during processing.
  uc_echeck_cache('save', $echeck_data, FALSE);

  // Build the data array passed on to the payment gateway.
  $data = array();

  $data['txn_type'] = UC_ECHECK_AUTH_CAPTURE;

  $result = uc_payment_process('echeck', $form_state['values']['order_id'], $form_state['values']['amount'], $data, TRUE, NULL, FALSE);

  if ($result) {
    $crypt = new uc_encryption_class();

    // Load up the existing data array.
    $data = db_result(db_query("SELECT data FROM {uc_orders} WHERE order_id = %d", $form_state['values']['order_id']));
    $data = unserialize($data);

    $cache = uc_echeck_cache('load');

    if (variable_get('uc_echeck_debug', FALSE) && !variable_get('uc_echeck_checkout_process', TRUE)) {
      $echeck_data = $cache;
    }
    else {
      $echeck_data = array(
        'echeck_bank_aba_code' => substr($cache['echeck_bank_aba_code'], -4),
        'echeck_bank_acct_num' => substr($cache['echeck_bank_acct_num'], -4),
      );
    }

    // Stuff the serialized and encrypted CC details into the array.
    $data['echeck_data'] = $crypt->encrypt(uc_echeck_encryption_key(), base64_encode(serialize($echeck_data)));
    uc_store_encryption_errors($crypt, 'uc_echeck');

    // Save it again.
    db_query("UPDATE {uc_orders} SET data = '%s' WHERE order_id = %d", serialize($data), $form_state['values']['order_id']);

    drupal_set_message(t('The eCheck was processed successfully. See the admin comments for more details.'));
  }
  else {
    if (variable_get('uc_echeck_debug', FALSE)) {
      _save_echeck_data_to_order(uc_credit_cache('load'), $form_state['values']['order_id']);
    }

    drupal_set_message(t('There was an error processing the eCheck.  See the admin comments for details.'), 'error');
  }

  $form_state['redirect'] = 'admin/store/orders/'. $form_state['values']['order_id'];
}

/**
 * Settings for the eCheck fieldset on the payment method settings form.
 *
 * @return
 *  The eCheck settings form.
 */

// Settings for the eCheck fieldset on the payment method settings form.
function uc_echeck_settings_form() {
  if (!user_access('administer echecks')) {
    $form['notice'] = array(
      '#value' => '<div>'. t('You must have access to <b>administer echecks</b> to adjust these settings.') .'</div>',
    );
    return $form;
  }

  $form['security'] = array(
    '#type' => 'fieldset',
    '#title' => t('Account number security'),
  );
  $form['security']['uc_echeck_encryption_path'] = array(
    '#type' => 'textfield',
    '#title' => t('Account number encryption key filepath'),
    '#description' => t('<b>You must enable encryption</b> by following the <a href="!url">encryption instructions</a> in order to protect customer bank account data.<br />In short, you should specify a path outside of your document root where the encryption key may be stored.<br />Relative paths will be resolved relative to the Drupal installation directory.<br />Once this is set, you should not change it.', array('!url' => 'http://www.ubercart.org/docs/user/2731/credit_card_settings#security')),
    '#default_value' => variable_get('uc_echeck_encryption_path', t('Not configured, see below.')),
  );
  $form['security']['uc_echeck_debug'] = array(
    '#type' => 'checkbox',
    '#title' => t('Operate in eCheck debug mode.'),
    '#description' => t('In debug mode, the full eCheck account number is stored which may be in violation of PCI security standards.<br />Debug mode is only recommended for testing transactions with fake account details.'),
    '#default_value' => variable_get('uc_echeck_debug', FALSE),
  );

  // Form elements to handle the automatic clearing of eCheck data.
  $form['echeck_clear'] = array(
    '#type' => 'fieldset',
    '#title' => t('Debug mode data clearing'),
    '#description' => t('Specify below the status and age of orders whose eCheck details will be removed.  This setting only applies when operating in debug mode. <strong>Cron must be running for this feature to work.</strong>', array('!url' => url('admin/store/settings/cart/edit'))),
    '#collapsible' => FALSE,
  );
  foreach (uc_order_status_list() as $status) {
    $options[$status['id']] = $status['title'];
  }
  $form['echeck_clear']['uc_echeck_clear_status'] = array(
    '#type' => 'select',
    '#title' => t('Order status'),
    '#options' => $options,
    '#default_value' => variable_get('uc_echeck_clear_status', uc_order_state_default('completed')),
    '#prefix' => '<div style="float: left; margin-right: 1em;">',
    '#suffix' => '</div>',
  );
  $form['echeck_clear']['uc_echeck_number_duration'] = array(
    '#type' => 'select',
    '#title' => t('Age'),
    '#options' => drupal_map_assoc(range(1, 24)),
    '#default_value' => variable_get('uc_echeck_number_duration', '3'),
    '#prefix' => '<div style="float: left; margin-right: 1em;">',
    '#suffix' => '</div>',
  );
  $form['echeck_clear']['uc_echeck_number_unit'] = array(
    '#type' => 'select',
    '#title' => t('Unit of time'),
    '#options' => array(
      'hours' => t('hour(s)'),
      'days' => t('day(s)'),
      'weeks' => t('week(s)'),
      'years' => t('year(s)'),
    ),
    '#default_value' => variable_get('uc_echeck_number_unit', 'days'),
    '#prefix' => '<div style="float: left;">',
    '#suffix' => '</div>',
  );

  // Form elements that deal with credit card messages to customers.
  $form['echeck_messages'] = array(
    '#type' => 'fieldset',
    '#title' => t('Customer messages'),
    '#description' => t('Here you can alter messages displayed to customers using eCheck.'),
    '#collapsible' => FALSE,
  );
  $form['echeck_messages']['uc_echeck_policy'] = array(
    '#type' => 'textarea',
    '#title' => t('eCheck payment policy'),
    '#description' => t('Instructions for customers on the checkout page above the eCheck fields.'),
    '#default_value' => variable_get('uc_echeck_policy', t('By selecting eCheck, you hereby authorize a direct withdrawal from the bank account indicated below.')),
    '#rows' => 3,
  );
  $form['echeck_messages']['uc_echeck_fail_message'] = array(
    '#type' => 'textarea',
    '#title' => t('eCheck processing failure message'),
    '#description' => t('Error message displayed to customers when an attempted payment fails at checkout.'),
    '#default_value' => variable_get('uc_echeck_fail_message', t('We were unable to process your eCheck payment. Please verify your bank details details and try again.  If the problem persists, contact us to complete your order.')),
  );
  
  return $form;
}

/**
 * Validates the encryption key directory and key file.
 *
 * Checks that the encryption key directory has been specified, that it
 * exists, and is readable. and writeable so
 *
 * @see uc_echeck_settings_form()
 * @see uc_echeck_settings_form_submit()
 */
function uc_echeck_settings_form_validate($form, &$form_state) {

  // Trim trailing whitespace and any trailing / or \ from the key path name.
  $key_path = rtrim(trim($form_state['values']['uc_echeck_encryption_path']), '/\\');

  // Test to see if a path was entered.
  if (empty($key_path)) {
    form_set_error('uc_echeck_encryption_path', t('Key path must be specified in security settings tab.'));
  }

  // Construct complete key file path.
  $key_file = $key_path . '/' . UC_ECHECK_KEYFILE_NAME;

  // Shortcut - test to see if we already have a usable key file.
  if (file_exists($key_file)) {
    if (is_readable($key_file)) {
      // Test contents - must contain 32-character hexadecimal string.
      $key = uc_echeck_encryption_key();
      if ($key) {
        if (!preg_match("([0-9a-fA-F]{32})", $key)) {
          form_set_error('uc_echeck_encryption_path', t('Key file already exists in directory, but it contains an invalid key.'));
        }
        else {
          // Key file exists and is valid, save result of trim() back into
          // $form_state and proceed to submit handler.
          $form_state['values']['uc_echeck_encryption_path'] = $key_path;
          return;
        }
      }
    }
    else {
      form_set_error('uc_echeck_encryption_path', t('Key file already exists in directory, but is not readable. Please verify the file permissions.'));
    }
  }

  // Check if directory exists and is writeable.
  if (is_dir($key_path)) {
    // The entered directory is valid and in need of a key file.
    // Flag this condition for the submit handler.
    $form_state['uc_echeck']['update_echeck_encrypt_dir'] = TRUE;

    // Can we open for writing?
    $file = @fopen($key_path . '/encrypt.test', 'w');
    if ($file === FALSE) {
      form_set_error('uc_echeck_encryption_path', t('Cannot write to directory, please verify the directory permissions.'));
      unset($form_state['uc_echeck']['update_echeck_encrypt_dir']);
    }
    else {
      // Can we actually write?
      if (@fwrite($file, '0123456789') === FALSE) {
        form_set_error('uc_echeck_encryption_path', t('Cannot write to directory, please verify the directory permissions.'));
        unset($form_state['uc_echeck']['update_echeck_encrypt_dir']);
        fclose($file);
      }
      else {
        // Can we read now?
        fclose($file);
        $file = @fopen($key_path . '/encrypt.test', 'r');
        if ($file === FALSE) {
          form_set_error('uc_echeck_encryption_path', t('Cannot read from directory, please verify the directory permissions.'));
          unset($form_state['uc_echeck']['update_echeck_encrypt_dir']);
        }
        else {
          fclose($file);
        }
      }
      unlink($key_path . '/encrypt.test');
    }
  }
  else {
    // Directory doesn't exist.
    form_set_error('uc_echeck_encryption_path', t('You have specified a non-existent directory.'));
  }

  // If validation succeeds, save result of trim() back into $form_state.
  $form_state['values']['uc_echeck_encryption_path'] = $key_path;
}

/**
 * Creates the encryption key file if it doesn't already exist.
 *
 * @see uc_echeck_settings_form()
 * @see uc_echeck_settings_form_validate()
 */
function uc_echeck_settings_form_submit($form, &$form_state) {
  // Check to see if we need to create a key file.
  if (isset($form_state['uc_echeck']['update_echeck_encrypt_dir']) &&
      $form_state['uc_echeck']['update_echeck_encrypt_dir'] === TRUE) {

    $key_path = $form_state['values']['uc_echeck_encryption_path'];
    $key_file = $key_path . '/' . UC_ECHECK_KEYFILE_NAME;

    if (!file_exists($key_file)) {
      if (!$file = fopen($key_file, 'wb')) {
        drupal_set_message(t('eCheck encryption key file creation failed for file @file. Check your filepath settings and directory permissions.', array('@file' => $key_file)), 'error');
        watchdog('uc_echeck', 'eCheck encryption key file creation failed for file @file. Check your filepath settings and directory permissions.', array('@file' => $key_file), WATCHDOG_ERROR);
      }
      else {
        // Replacement key generation suggested by Barry Jaspan
        // for increased security.
        fwrite($file, md5(drupal_get_token(serialize($_REQUEST) . serialize($_SERVER) . time())));
        fclose($file);

        drupal_set_message(t('eCheck encryption key file generated. eCheck data will now be encrypted.'));
        watchdog('uc_echeck', 'eCheck encryption key file generated. eCheck data will now be encrypted.');
      }
    }
  }
}