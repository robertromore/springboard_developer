<?php
/**
 * @file
 * Theme functions for Springboard Developer.
 */

/**
 * Theme hook.
 */
function theme_springboard_developer_ajax_table(&$variables) {
  $form = &$variables['form'];
  dpm($form);
  $rows = array();

  foreach ($form['#rows'] as $row) {
    dpm($row);
    $buttons = '';
    foreach ($row['data'][1]['data'] as $data) {
      $buttons .= drupal_render($data);
    }
    $rows[] = array($row['data'][0], $buttons);
  }

  $output = theme('table', array(
    'header' => $form['#header'],
    'rows' => $rows,
  ));

  return $output . drupal_render_children($form);
}
