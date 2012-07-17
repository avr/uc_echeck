<?php

// Form to gather the required fields to process an eCheck payment.
function uc_echeck_payment_form($form_state, $order) {
  $form = array();

  $form['echeck_bank_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Bank name'),
    '#default_value' => $order->payment_details['echeck_bank_name'],
    '#attributes' => array('autocomplete' => 'off'),
    '#required' => TRUE,
    '#maxlength' => 50,
    '#size' => 32,
  );
  $form['echeck_bank_aba_code'] = array(
    '#type' => 'textfield',
    '#title' => t('Bank routing number'),
    '#description' => t('The first 9 digit number along the bottom of your checks.'),
    '#default_value' => $order->payment_details['echeck_bank_aba_code'],
    '#attributes' => array('autocomplete' => 'off'),
    '#required' => TRUE,
    '#maxlength' => 9,
    '#size' => 32,
  );
  $form['echeck_bank_acct_num'] = array(
    '#type' => 'textfield',
    '#title' => t('Bank account number'),
    '#description' => t('Up to a 20 digit number next to the the routing number.'),
    '#default_value' => $order->payment_details['echeck_bank_acct_num'],
    '#attributes' => array('autocomplete' => 'off'),
    '#required' => TRUE,
    '#maxlength' => 20,
    '#size' => 32,
  );
  $form['echeck_bank_acct_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name on the account'),
    '#default_value' => $order->payment_details['echeck_bank_acct_name'],
    '#attributes' => array('autocomplete' => 'off'),
    '#required' => TRUE,
    '#maxlength' => 50,
    '#size' => 32,
  );
  $form['echeck_bank_acct_type'] = array(
    '#type' => 'select',
    '#title' => t('Bank account type'),
    '#options' => uc_echeck_acct_type(),
    '#default_value' => !empty($order->payment_details['echeck_bank_acct_type']) ? $order->payment_details['echeck_bank_acct_type'] : 'CHECKING',
    '#required' => TRUE,
  );

  return $form;
}

// Settings for the eCheck fieldset on the payment method settings form.
function uc_echeck_settings_form() {

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
  $form['security']['uc_echeck_acct_num_mask'] = array(
    '#type' => 'textfield',
    '#title' => t('Account number mask'),
    '#description' => t('To protect bank account numbers, the last 4 digits will be displayed after this mask when they are shown to the user.'),
    '#default_value' => variable_get('uc_echeck_acct_num_mask', 'xxxxxx'),
  );
  $form['security']['uc_echeck_wipe_interval'] = array(
    '#type' => 'select',
    '#title' => t('Data wipe interval'),
    '#description' => t('eCheck data will be wiped on inactive orders that are canceled or never make it through checkout after this interval of time passes.'),
    '#options' => array(
      300 => t('5 minutes'),
      600 => t('10 minutes'),
      900 => t('15 minutes'),
      1200 => t('20 minutes'),
      1500 => t('25 minutes'),
      1800 => t('30 minutes'),
      2700 => t('45 minutes'),
      3600 => t('1 hour'),
      7200 => t('2 hours'),
      14400 => t('4 hours'),
      28800 => t('8 hours'),
      57600 => t('16 hours'),
      86400 => t('1 day'),
    ),
    '#default_value' => variable_get('uc_echeck_wipe_interval', 900),
  );
  // Form elements that deal with credit card messages to customers.
  $form['echeck_messages'] = array(
    '#type' => 'fieldset',
    '#title' => t('Customer messages'),
    '#description' => t('Here you can alter messages displayed to customers using credit cards.'),
    '#collapsible' => FALSE,
  );
  $form['echeck_messages']['uc_echeck_policy'] = array(
    '#type' => 'textarea',
    '#title' => t('eCheck payment policy'),
    '#description' => t('Instructions for customers on the checkout page above the eCheck fields.'),
    '#default_value' => variable_get('uc_echeck_policy', t('By selecting eCheck, you hereby authorize a direct withdrawal from the bank account indicated below.')),
    '#rows' => 3,
  );
  $form['cc_messages']['uc_echeck_fail_message'] = array(
    '#type' => 'textarea',
    '#title' => t('eCheck processing failure message'),
    '#description' => t('Error message displayed to customers when an attempted payment fails at checkout.'),
    '#default_value' => variable_get('uc_echeck_fail_message', t('We were unable to process your eCheck payment. Please verify your bank details details and try again.  If the problem persists, contact us to complete your order.')),
  );
  

  return $form;
}