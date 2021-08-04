<?php

/**
 * @file
 * Theme callbacks to pre-tag rows for FPA functionality.
 */

/**
 * Implements hook_theme().
 */
function fpa_theme($existing, $type, $theme, $path) {
  return [
    'fpa_user_admin_permissions' => [
      'render element' => 'form',
      'file' => 'fpa.theme.inc',
    ],
  ];
}

/**
 * Theme function to pre-mark rows with FPA attributes.
 *
 * Based on Drupal Core's permissions form theme function.
 *
 * @see theme_user_admin_permissions().
 */
function theme_fpa_user_admin_permissions($variables) {
  $form = $variables['form'];

  $nameless_checkbox = [
    '#type' => 'html_tag',
    '#tag' => 'input',
    '#attributes' => [
      'type' => 'checkbox',
      'class' => [
        'rid-1', // Prevents Drupal core Drupal.behaviors.permissions.toggle from applying.
        'form-checkbox',
        'fpa-checkboxes-toggle',
      ],
    ],
  ];

  $nameless_checkbox_output = \Drupal::service('renderer')->render($nameless_checkbox);

  $dummy_checkbox = [
    '#type' => 'html_tag',
    '#tag' => 'input',
    '#attributes' => [
      'type' => 'checkbox',
      'disabled' => 'disabled',
      'checked' => 'checked',
      'title' => t('This permission is inherited from the authenticated user role.'),
      'class' => [
        'dummy-checkbox',
      ],
    ],
  ];

  $dummy_checkbox_output = \Drupal::service('renderer')->render($dummy_checkbox);

  $permission_col_template = [
    '#type' => 'container',
    '#attributes' => [
      'class' => [
        'fpa-permission-container',
      ],
    ],
    'description' => [
      '#markup' => '',
    ],
    'checkbox_cell' => [
      '#type' => 'container',
      '#attributes' => [
        'class' => [
          'fpa-row-toggle-container',
        ],
      ],
      'checkbox_form_item' => [
        '#type' => 'container',
        '#attributes' => [
          'title' => t('Toggle visible checkboxes in this row.'),
          'class' => [
            'form-item',
            'form-type-checkbox',
          ],
        ],
        'label' => [
          '#type' => 'html_tag',
          '#tag' => 'label',
          '#attributes' => [
            'class' => [
              'element-invisible',
            ],
          ],
          '#value' => 'test',
        ],
        'checkbox' => [
          '#markup' => $nameless_checkbox_output,
        ],
      ],
    ],
  ];

  $checkboxes_children = element_children($form['checkboxes']);

  // Prepare role names processed by drupal_html_class() ahead of time.
  $roles_attr_values = [];

  foreach ($checkboxes_children as $rid) {
    $roles_attr_values[$rid] = drupal_html_class($form['role_names'][$rid]['#markup']);
  }

  $first_role_index = array_shift($checkboxes_children);

  // Lists for wrapper.
  $modules = [];
  $user_roles = [];

  // Index of current module row.
  $module = NULL;

  // Row counter.
  $i = 0;

  $rows = [];

  // Iterate over rows in form table.
  foreach (element_children($form['permission']) as $key) {

    // Row template.
    $row = [
      'data' => [], // Array of table cells.
      'title' => [], // HTML attribute on table row tag.
      FPA_ATTR_MODULE => [], // HTML attribute on table row tag.
      FPA_ATTR_PERMISSION => [], // HTML attribute on table row tag.
      FPA_ATTR_CHECKED => [],
      FPA_ATTR_NOT_CHECKED => [],
    ];

    // Determine if row is module or permission.
    if (is_numeric($key)) {
      // Module row.

      $row['class'][] = 'fpa-module-row';

      // Mark current row with escaped module name.
      $row[FPA_ATTR_MODULE] = [
        // System name
        0 => $form['permission'][$key]['#id'],
        // Readable name
        1 => strip_tags($form['permission'][$key]['#markup']),
      ];

      // Readable
      $row['data'][] = [
        'data' => \Drupal::service('renderer')->render($form['permission'][$key]),
        'class' => ['module'],
        'id' => 'module-' . $form['permission'][$key]['#id'],
        'colspan' => count($form['role_names']['#value']) + 1,
      ];

      $row['title'] = [$form['permission'][$key]['#id']];

      $row[FPA_ATTR_SYSTEM_NAME] = $row[FPA_ATTR_MODULE][0];

      $row[FPA_ATTR_MODULE] = array_unique(array_map('drupal_html_class', $row[FPA_ATTR_MODULE]));

      // Add modules to left-side modules list.
      $modules[$row[FPA_ATTR_MODULE][0]] = [
        'text' => strip_tags($form['permission'][$key]['#markup']),
        'title' => [$form['permission'][$key]['#id']],
        FPA_ATTR_MODULE => $row[FPA_ATTR_MODULE],
        FPA_ATTR_PERMISSION => [],
      ];

      // Save row number for current module.
      $module = $i;
    }
    else {
      // Permission row.

      $row['class'][] = 'fpa-permission-row';

      $permission_system_name = '';

      // Might be empty if no modules are displayed in Permissions Filter module.
      if (!empty($form['checkboxes'][$first_role_index])) {
        $permission_system_name = $form['checkboxes'][$first_role_index][$key]['#return_value'];
      }

      $label = $permission_col_template;

      $label['description']['#markup'] = \Drupal::service('renderer')->render($form['permission'][$key]);

      // Permissions filter might cause no Roles to display.
      if (count(element_children($form['checkboxes'])) == 0) {
        unset($label['checkbox_cell']);
      }

      // Readable
      $row['data'][] = [
        'data' => \Drupal::service('renderer')->render($label),
        'class' => ['permission'],
      ];

      foreach (element_children($form['checkboxes']) as $rid) {
        $form['checkboxes'][$rid][$key]['#title'] = $form['role_names'][$rid]['#markup'] . ': ' . $form['permission'][$key]['#markup'];
        $form['checkboxes'][$rid][$key]['#title_display'] = 'invisible';

        // Filter permissions strips role id class from checkbox. Used by Drupal core functionality.
        $form['checkboxes'][$rid][$key]['#attributes']['class'][] = 'rid-' . $rid;

        // Set authenticated role behavior class on page load.
        if ($rid == 2 && $form['checkboxes'][$rid][$key]['#checked'] === TRUE) {
          $row['class'][] = 'fpa-authenticated-role-behavior';
        }

        // For all roles that inherit permissions from 'authenticated user' role, add in dummy checkbox for authenticated role behavior.
        if ($rid > 2) {
          $form['checkboxes'][$rid][$key]['#suffix'] = $dummy_checkbox_output; // '#suffix' doesn't have wrapping HTML like '#field_suffix'.
        }

        // Add rid's to row attribute for checked status filter.
        if ($form['checkboxes'][$rid][$key]['#checked'] === TRUE) {
          $row[FPA_ATTR_CHECKED][] = $rid;
        }
        else {
          $row[FPA_ATTR_NOT_CHECKED][] = $rid;
        }

        $row['data'][] = [
          'data' => \Drupal::service('renderer')->render($form['checkboxes'][$rid][$key]),
          'class' => [
            'checkbox',
          ],
          'title' => [
            $form['role_names'][$rid]['#markup'],
          ],
          // For role filter
          FPA_ATTR_ROLE => [
            $rid,
          ],
        ];
      }

      if (!empty($rid)) {
        $row['title'] = [
          $form['checkboxes'][$rid][$key]['#return_value'],
        ];

        $row[FPA_ATTR_SYSTEM_NAME] = [
          $form['checkboxes'][$rid][$key]['#return_value'],
        ];
      }

      // Mark current row with escaped permission name.
      $row[FPA_ATTR_PERMISSION] = [
        // Permission system name.
        0 => $permission_system_name,
        // Readable description.
        1 => strip_tags($form['permission'][$key]['#markup']),
      ];

      // Mark current row with current module.
      $row[FPA_ATTR_MODULE] = $rows[$module][FPA_ATTR_MODULE];

      $row[FPA_ATTR_PERMISSION] = array_unique(array_map('drupal_html_class', $row[FPA_ATTR_PERMISSION]));

      // Add current permission to current module row.
      $rows[$module][FPA_ATTR_PERMISSION] = array_merge($rows[$module][FPA_ATTR_PERMISSION], $row[FPA_ATTR_PERMISSION]);

      $rows[$module][FPA_ATTR_CHECKED] = array_unique(array_merge($rows[$module][FPA_ATTR_CHECKED], $row[FPA_ATTR_CHECKED]));
      $rows[$module][FPA_ATTR_NOT_CHECKED] = array_unique(array_merge($rows[$module][FPA_ATTR_NOT_CHECKED], $row[FPA_ATTR_NOT_CHECKED]));

      $modules[$rows[$module][FPA_ATTR_MODULE][0]][FPA_ATTR_PERMISSION][] = $row[FPA_ATTR_PERMISSION];
    }

    $rows[$i++] = $row;
  }

  $reset_button = [
    '#type' => 'html_tag',
    '#tag' => 'input',
    '#attributes' => [
      'type' => 'reset',
      'class' => 'form-submit',
      'value' => t('Reset changes'),
    ],
  ];

  // If there is no submit button, don't add the reset button.
  if (count(element_children($form['actions'])) > 0) {

    // Have the reset button appear before the submit button.
    array_unshift($form['actions'], $reset_button);
  }

  foreach ($form['actions'] as $key) {
    if (!empty($form['actions'][$key])) {
      $actions_output .= \Drupal::service('renderer')->render($form['actions'][$key]);
    }
  }
  $header = [];

  $header[] = [
    'data' => t('Permission') . $actions_output,
  ];

  foreach (element_children($form['role_names']) as $rid) {

    $header[] = [
      'data' => \Drupal::service('renderer')->render($form['role_names'][$rid]) . $nameless_checkbox_output,
      'class' => [
        'checkbox',
      ],
      'title' => [
        $form['role_names'][$rid]['#markup'],
      ],
      FPA_ATTR_ROLE => [
        $rid,
      ],
    ];
    $user_roles[$rid] = $form['role_names'][$rid]['#markup'];
  }

  $table = [
    'header' => $header,
    'rows' => $rows,
  ];

  $output = _fpa_wrapper($table, $modules, $user_roles, $actions_output);

  foreach ($form as $key) {
    if (!empty($form[$key])) {
      $output .= \Drupal::service('renderer')->render($form[$key]);
    }
  }

  return $output;
}

/**
 * Wraps table output in the FPA filter.
 */
function _fpa_wrapper($permissions_table, $modules, $user_roles, $actions_output) {

  $same_page = trim(parse_url($_SERVER['HTTP_REFERER'], PHP_URL_PATH), '/') == $_GET['q'];

  $render = [
    '#type' => 'container',
    '#attributes' => [
      'class' => [
        'fpa-container',
      ],
    ],
  ];

  $hiders = [
    'fpa-hide-descriptions' => [
      'hide' => t('Hide descriptions'),
      'show' => t('Show descriptions'),
    ],
    'fpa-hide-system-names' => [
      'hide' => t('Hide system names'),
      'show' => t('Show system names'),
    ],
  ];

  $render['#attributes']['class'][] = 'fpa-hide-system-names';

  $hide_container = [
    '#type' => 'container',
    '#attributes' => [
      'class' => [
        'fpa-toggle-container',
      ],
    ],
  ];

  foreach ($hiders as $hide_class => $labels) {

    $hide_container[$hide_class] = [
      '#theme' => 'link',
      '#text' => '',
      '#path' => '',
      '#options' => [
        'attributes' => array_merge($labels, [
          'fpa-toggle-class' => $hide_class,
        ]),
        'html' => TRUE,
        'fragment' => ' ',
        'external' => TRUE, // Prevent base path from being added to link.
      ],
    ];
  }

  $render['hide_container'] = $hide_container;

  $wrapper = [
    '#type' => 'container',
    '#attributes' => [
      'class' => [
        'fpa-wrapper',
      ],
    ],
  ];

  $render['wrapper'] = &$wrapper;

  /**
   * <style /> block template.
   */
  $style_template = [
    '#type' => 'container',
    '#attributes' => [
      'class' => [
        'style-wrapper-class-name', // Override on specific block.
      ],
    ],
  ];

  $style_template['style'] = [
    '#type' => 'html_tag',
    '#tag' => 'style',
    '#attributes' => [
      'type' => [
        'text/css',
      ],
    ],
    '#value' => '', // #value needed for closing tag.
  ];

  /**
   * <style /> block for role filtering.
   */
  $wrapper['role_styles'] = $style_template;
  $wrapper['role_styles']['#attributes']['class'][0] = 'fpa-role-styles';

  /**
   * <style /> block for permission filtering.
   */
  $wrapper['perm_styles'] = $style_template;
  $wrapper['perm_styles']['#attributes']['class'][0] = 'fpa-perm-styles';

  /**
   * Left section contains module list and form submission button.
   */
  $left_section = [
    '#type' => 'container',
    '#attributes' => [
      'class' => [
        'fpa-left-section',
      ],
    ],
  ];

  $wrapper['left_section'] = &$left_section;


  /**
   * Right section contains filter form and permissions table.
   */
  $right_section = [
    '#type' => 'container',
    '#attributes' => [
      'class' => [
        'fpa-right-section',
      ],
    ],
  ];

  $wrapper['right_section'] = &$right_section;

  $module_template = [
    FPA_ATTR_MODULE => [],
    FPA_ATTR_PERMISSION => [],
    'data' => [
      '#type' => 'container',
      '#attributes' => [],

      'link' => [
        '#type' => 'markup',
        '#markup' => '', // l($module['text'], 'admin/people/permissions', $options)
      ],

      'counters' => [],

      'total' => [
        '#type' => 'html_tag',
        '#tag' => 'span',
        '#attributes' => [
          'class' => ['fpa-perm-total'],
          'fpa-total' => 0,
        ],
        '#value' => '', // #value needed for closing tag.
      ],
    ],
  ];

  $counter_template = [
    '#type' => 'html_tag',
    '#tag' => 'span',
    '#attributes' => [
      'class' => ['fpa-perm-counter'],
      FPA_ATTR_PERMISSION => [], // Counters only count permissions match.
    ],
    '#value' => '', // #value required for closing tag.
  ];

  $items = [];

  $all_modules = [
    'text' => t('All modules'),
    FPA_ATTR_MODULE => [],
    FPA_ATTR_PERMISSION => [],
  ];

  array_unshift($modules, $all_modules);

  $all_modules_counters = [];

  foreach ($modules as $module) {

    $module_item = $module_template;

    $module_item[FPA_ATTR_MODULE] = $module[FPA_ATTR_MODULE];
    $module_item[FPA_ATTR_PERMISSION] = array_reduce(array_pad($module[FPA_ATTR_PERMISSION], 1, []), 'array_merge', []);

    // Use link for accessibility and tabability.
    $options = [
      'fragment' => 'all',
    ];

    if (!empty($module['title'])) {
      $options['fragment'] = 'module-' . $module['title'][0];
      $options['attributes']['title'] = $module['title'][0];
    }

    $module_item['data']['link']['#markup'] = l($module['text'], 'admin/people/permissions', $options);

    foreach ($module[FPA_ATTR_PERMISSION] as $module_perm) {

      $counter_item = $counter_template;

      $counter_item['#attributes'][FPA_ATTR_PERMISSION] = $module_perm;

      $all_modules_counters[] = $counter_item;

      $module_item['data']['counters'][] = $counter_item;
    }

    $module_item['data']['total']['#attributes']['fpa-total'] = count($module[FPA_ATTR_PERMISSION]);

    $items[] = $module_item;
  }

  $items[0]['data']['counters'] = $all_modules_counters;
  $items[0]['data']['total']['#attributes']['fpa-total'] = count($all_modules_counters);

  foreach ($items as &$item) {
    $item['data'] = \Drupal::service('renderer')->render($item['data']);
  }

  $left_section['list'] = [
    '#items' => $items,
    '#theme' => 'item_list',
  ];

  $left_section['buttons'] = [
    '#type' => 'markup',
    '#markup' => $actions_output,
  ];

  $filter_form = [
    '#type' => 'container',
    '#attributes' => [
      'class' => [
        'fpa-filter-form',
      ],
    ],
  ];

  $clear_button = [
    '#type' => 'html_tag',
    '#tag' => 'input',
    '#attributes' => [
      'type' => [
        'button',
      ],
      'class' => [
        'fpa-clear-search',
        'form-submit',
      ],
      'value' => 'Clear filter',
    ],
  ];

  $default_filter = '';

  if (!empty($_GET['fpa_perm'])) {
    $default_filter = $_GET['fpa_perm'];
  }

  if (!empty($_COOKIE['fpa_filter']) && $same_page) {
    $default_filter = $_COOKIE['fpa_filter'];
  }

  $filter_form['permission_module_filter'] = [
    '#type' => 'textfield',
    '#title' => t('Filter:'),
    '#description' => t('<p>Enter in the format of "permission@module",</p><p>e.g. <em>admin@system</em> will show only permissions with the<br>text "admin" in modules with the text "system".</p><p>This will also match on system name of a permission.</p>'),
    '#size' => 25,
    '#field_suffix' => \Drupal::service('renderer')->render($clear_button),
    '#attributes' => [
      'placeholder' => [
        'permission@module',
      ],
      'autofocus' => 'autofocus',
    ],
    '#value' => $default_filter,
  ];

  /*
   * Populate the permission filter styles.
   */
  $matches = [];

  preg_match('/^\s*([^@]*)@?(.*?)\s*$/i', $filter_form['permission_module_filter']['#value'], $matches);

  array_shift($matches); // Remove whole match item.

  $safe_matches = array_map('drupal_html_class', $matches);

  $module_match = !empty($_COOKIE['module_match']) ? $_COOKIE['module_match'] : '*=';

  $filters = [
    drupal_strlen($safe_matches[0]) > 0 ? ('[' . FPA_ATTR_PERMISSION . '*="' . $safe_matches[0] . '"]') : '',
    drupal_strlen($safe_matches[1]) > 0 ? ('[' . FPA_ATTR_MODULE . $module_match . '"' . $safe_matches[1] . '"]') : '',
  ];

  $filter_styles = [
    '.fpa-table-wrapper tr[' . FPA_ATTR_MODULE . ']{display: none;}',

    '.fpa-table-wrapper tr[' . FPA_ATTR_MODULE . ']',
    $filters[0],
    $filters[1],
    '{display: table-row;}',


    '.fpa-perm-counter{display: none;}',
    '.fpa-perm-counter',
    $filters[0],
    '{display: inline;}',


    '.fpa-left-section li[' . FPA_ATTR_MODULE . ']',
    drupal_strlen($filters[1]) > 0 ? $filters[1] : '[' . FPA_ATTR_MODULE . '=""]',
    '{margin-right:-1px; background-color: white; border-right: solid 1px transparent;}',
  ];

  $wrapper['perm_styles']['style']['#value'] = implode('', $filter_styles);


  $cookie_roles = (!empty($_COOKIE['fpa_roles']) && $same_page) ? json_decode($_COOKIE['fpa_roles']) : [];

  $options = [
    '*' => t('--All Roles'),
  ];

  if (!empty($user_roles)) {
    $options += $user_roles; // Preserves keys.
  }

  if (in_array('*', $cookie_roles)) {
    $cookie_roles = ['*'];
  }

  $filter_form['role_filter'] = [
    '#type' => 'select',
    '#title' => t('Roles:'),
    '#description' => t('Select which roles to display.<br>Ctrl+click to select multiple.'),
    '#size' => 5,
    '#options' => $options,
    '#attributes' => [
      'multiple' => 'multiple',
      'autocomplete' => 'off', // Keep browser from populating this from 'cached' input.
    ],
    '#value' => count(array_intersect($cookie_roles, array_keys($options))) > 0 ? $cookie_roles : ['*'],
  ];

  /*
   * Populate the roles styles.
   */
  if (!in_array('*', $filter_form['role_filter']['#value'])) {

    $role_styles = [
      '.fpa-table-wrapper [' . FPA_ATTR_ROLE . '] {display: none;}',
    ];

    foreach ($filter_form['role_filter']['#value'] as $value) {

      $role_styles[] = '.fpa-table-wrapper [' . FPA_ATTR_ROLE . '="' . $value . '"] {display: table-cell;}';
    }

    $role_styles[] = '.fpa-table-wrapper [' . FPA_ATTR_ROLE . '="' . end($filter_form['role_filter']['#value']) . '"] {border-right: 1px solid #bebfb9;}';

    $wrapper['role_styles']['style']['#value'] = implode('', $role_styles);
  }

  $checked_status = [
    '#type' => 'checkboxes',
    '#title' => t('Display permissions that are:'),
    '#options' => [
      FPA_ATTR_CHECKED => t('Checked'),
      FPA_ATTR_NOT_CHECKED => t('Not Checked'),
    ],
    '#attributes' => [],
    '#title_display' => 'before',
    '#description' => t('Applies to all visible roles.<br />Unsaved changes are not counted.<br />Most effective when a single role is visible.<br />Empty module rows sometimes display when used with permission filter.'),
  ];

  $checked_status_keys = array_keys($checked_status['#options']);

  $checked_status['#value'] = array_combine($checked_status_keys, $checked_status_keys);

  $filter_form['checked_status'] = form_process_checkboxes($checked_status);

  foreach (element_children($filter_form['checked_status']) as $key) {
    $filter_form['checked_status'][$key]['#checked'] = TRUE;
    $filter_form['checked_status'][$key]['#id'] = drupal_html_id('edit-checkboxes-' . $key);
  }

  $right_section['filter_form'] = $filter_form;

  $table_wrapper = [
    '#type' => 'container',
    '#attributes' => [
      'class' => [
        'fpa-table-wrapper',
      ],
    ],
  ];

  $table_wrapper['table'] = [
    '#theme' => 'table',
    '#header' => $permissions_table['header'],
    '#rows' => $permissions_table['rows'],
    '#attributes' => [
      'id' => 'permissions',
    ],
  ];

  // Show after full table HTML is loaded. Reduces progressive table load reflow/repaint.
  $table_wrapper['show_table'] = [
    '#type' => 'html_tag',
    '#tag' => 'style',
    '#attributes' => [
      'type' => [
        'text/css',
      ],
    ],
    '#value' => '#permissions {display: table;} .fpa-table-wrapper {background: none;}',
  ];

  $table_wrapper['buttons'] = [
    '#type' => 'markup',
    '#markup' => $actions_output,
  ];

  $right_section['table_wrapper'] = $table_wrapper;


  return \Drupal::service('renderer')->render($render);
}
