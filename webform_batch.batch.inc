<?php

/**
 * @file
 * Batch operations.
 */

/**
 * Batch Export as CSV.
 */
function webform_batch_export_csv($nid, $limit, &$context) {
  $node = node_load($nid);
  
  // Processing hasn't started, initialize sandbox.
  if ( !isset($context['sandbox']['progress']) ) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['components'] = 0;
    $context['sandbox']['max'] = db_result(db_query('SELECT COUNT(DISTINCT sid) FROM {webform_submissions} WHERE nid = %d', $node->nid));
    $context['results']['filename'] = 'webform_batch_export_' . $node->nid . '.csv';
    $context['results']['temp_file'] = file_directory_temp() . '/' . $context['results']['filename'];
    
    // Create temporary webform batch export file.
    $f = fopen($context['results']['temp_file'], 'w');
    
    // Get default headers.
    $header = array(t('SID'), t('Time'), t('IP Address'), t('UID'), t('Username'));
    
    foreach ( $node->webform['components'] as $cid => $component ) {
      $csv_header_function = '_webform_csv_headers_' . $component['type'];
      
      if ( function_exists($csv_header_function) ) {
        $component_header = $csv_header_function($component, $component);
        
        if ( is_array($component_header[2]) ) {
          $component_header[2] = $component_header[2][0];
        }
        
        $header[] = check_plain(strip_tags($component_header[2]));
        $context['sandbox']['components']++;
      }
    }
    
    // Write header to file.
    fputcsv($f, $header);
  }
  else {
    // Open temporary webform batch export file to append data.
    $f = fopen($context['results']['temp_file'], 'a');
  }
  
  // Look for submissions.
  $sql  = "SELECT * FROM {webform_submissions} s ";
  $sql .= "WHERE s.nid = %d ";
  $sql .= "LIMIT " . $context['sandbox']['progress'] . ", " . $limit;
  
  $submissions = array();
  $result = db_query($sql, $node->nid);
  $results = db_affected_rows();
  while ( $row = db_fetch_object($result) ) {
    $submissions[$row->sid] = array();
    $submissions[$row->sid]['sid'] = $row->sid;
    $submissions[$row->sid]['submitted'] = $row->submitted;
    $submissions[$row->sid]['remote_addr'] = $row->remote_addr;
    $submissions[$row->sid]['uid'] = $row->uid;
    $submissions[$row->sid]['name'] = $row->name;
    
    $sql = "SELECT * FROM {webform_submitted_data} WHERE sid = %d";
    $result_data = db_query($sql, $row->sid);
    while ( $row2 = db_fetch_object($result_data) ) {
      $submissions[$row->sid]['data'][$row2->cid] = $row2->data;
    }
  }
  
  foreach ( $submissions as $sid => $submission ) {
    $row = array();
    $row[] = $sid;
    $row[] = format_date($submission['submitted'], 'small');
    $row[] = $submission['remote_addr'];
    $row[] = $submission['uid'];
    $row[] = $submission['name'];
    
    foreach ( $node->webform['components'] as $cid => $component ) {
      $row[] = $submission['data'][$cid];
    }
    
    // Append to temp file.
    fputcsv($f, $row);
  }
  
  // Close file.
  fclose($f);
  
  // Increase the progress.
  $context['sandbox']['progress'] = $context['sandbox']['progress'] + $results;
  $context['message'] = t('Exporting submissions @progress of @max', array('@progress' => $context['sandbox']['progress'], '@max' => $context['sandbox']['max']));
  
  // Inform the batch engine that we are not finished, and provide an estimation of the completion level we have reached.
  if ( $context['sandbox']['progress'] < $context['sandbox']['max'] ) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  } else {
    $context['finished'] = 1;
  }
}

/**
 * Process finished batch export.
 */
function webform_batch_finished($success, $results, $operations) {
  $source = $results['temp_file'];

  $dest = file_create_path() . "/webform_batch/" . $results['filename'];
  if ( !file_exists(file_create_path() . "/webform_batch/") ) {
    mkdir(file_create_path() . "/webform_batch/");
  }
  
  copy($source, $dest);
  
  drupal_set_message('Click <a href="/' . $dest . '">here</a> to download your results.');
}