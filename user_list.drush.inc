<?php

/**
 * @file
 *
 * Adds a drush command to list users.
 */


 /**
 * Implements hook_drush_command().
 */
function user_list_drush_command() {

  $items['user-list'] = array(
    'description' => 'Display a list of users.',
    'aliases' => array('uls'),
    'options' => array(
      'roles' => 'A comma sepparated list of roles to filter by',
      'status' => 'Filter by status of the account. Can be active or blocked',
    ),
    'examples' => array(
      'drush user-list --roles=admin,editor' => 'Displays a list of users with the roles admin or editor.',
      'drush user-list --status=blocked' => 'Displays a list of blocked users.',
    ),
    'core' => array(6),
  );

  return $items;
}

 /**
  * Displays a list of users.
  */
function drush_user_list() {
  // Get options passed.
  $roles = _convert_csv_to_array(drush_get_option('roles'));
  $status = drush_get_option('status');

  // The fields to grab from the users tables.
  $user_fields = array(
    'uid',
    'name',
    'mail',
    'status',
  );
  // The $uids array will be used for printing the table.
  $uids  = array();
  $where = NULL;
  $args  = NULL;
  $st    = '';

  if (!empty($status)) {
    if ($status == 'active') {
      $st = '1';
    }
    if ($status == 'blocked') {
      $st = '0';
    }
    if (isset($st)) {
      $where = "status = %d";
    }
  }

  // Get the RIDs of the users with the roles specified.
  if (!empty($roles)) {
    // Find out the rids for the role names specified.
    $placeholder = db_placeholders($roles, 'text');
    $rquery      = drush_db_select('role', 'rid', "name in ($placeholder)", $roles);
    $rids        = array();
    while ($rid = drush_db_fetch_object($rquery)) {
      $rids[] = $rid->rid;
    }

    // Find out the UIDs of the users with the RIDs speficied.
    $placeholder = db_placeholders($rids, 'int');
    $query = drush_db_select('users_roles', 'uid', "rid in ($placeholder)", $rids);
    while ($uid = drush_db_fetch_object($query)) {
      $uids[] = $uid->uid;
    }

    unset($rquery);
    unset($query);

    if (!empty($uids)) {
      $placeholder = db_placeholders($uids, 'int');
      $where = (empty($where)) ? "uid IN ($placeholder)" : $where ." AND uid IN ($placeholder)";
    }
    else {
      return drush_set_error(dt('There were no users found with the roles specified.'));
    }
  }

  $args = array_merge((array)$st, $uids);

  // Get all UIDs.
  $query = drush_db_select('users', $user_fields, $where, $args);

  // Build the table.
  $header = array('UID', dt('Name'), dt('Email'), dt('Roles'), dt('Status'));
  $rows[] = $header;

  // Load each users info and add it to the results table.
  while ($user = drush_db_fetch_object($query)) {
    if ($user->uid == '0') {
      continue;
    }
    $user_roles = array();
    // Find out the roles of the user.
    $urquery = drush_db_select('users_roles', 'rid', 'uid = %d', array($user->uid));
    while ($result = drush_db_fetch_object($urquery)) {
      // Find the specific role name.
      $rquery = drush_db_select('role', 'name', 'rid = %d', array($result->rid));
      while ($role = drush_db_fetch_object($rquery)) {
        $user_roles[] = $role->name;
      }
    }
    $user_status = ($user->status) ? 'Active' : 'Blocked';

    drush_print_pipe($user->mail ."\n");

    $user_roles = implode(', ', $user_roles);

    $user_row = array(
      $user->uid,
      $user->name,
      $user->mail,
      $user_roles,
      $user_status,
    );
    $rows[] = $user_row;
  }
  drush_print_table($rows, TRUE);
}


