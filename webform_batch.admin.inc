<?php

/**
 * @file
 * Administration settings.
 */

/**
 * Implements hook_admin_settings_form().
 */
function webform_batch_admin_settings_form() {
  $form = array();
  
  $form['webform_batch_number_results'] = array(
    '#type' => 'textfield',
    '#title' => t('Number of results to export at once'),
    '#default_value' => variable_get('webform_batch_number_results', 1000),
  );
  
  return system_settings_form($form);
}
