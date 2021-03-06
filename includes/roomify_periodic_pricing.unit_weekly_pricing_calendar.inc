<?php

/**
 * @file
 * Contains UnitWeeklyPricingCalendar.
 */

/**
 * Handles querying and updating the pricing information
 * relative to a single bookable unit.
 */
class UnitWeeklyPricingCalendar extends UnitPricingCalendar {

  /**
   * The actual unit relevant to this Calendar.
   */
  protected $unit;

  /**
   * The default price for the room
   *
   * @var float
   */
  protected $default_discount;

  /**
   * Price modifiers - an array of operations to be performed to the price.
   * Operations are performed in the sequence they are found in the array
   *
   * @var array
   */
  protected $price_modifiers;

  /**
   * Constructs a UnitPricingCalendar instance.
   *
   * @param int $unit_id
   *   The unit ID.
   * @param array $price_modifiers
   *   The price modifiers to apply.
   */
  public function __construct($unit_id, $price_modifiers = array()) {
    $this->unit_id = $unit_id;
    // Load the booking unit.
    $this->unit = rooms_unit_load($unit_id);
    $this->default_state = $this->unit->default_state;
    $this->default_discount = $this->unit->weekly_discount;

    $this->price_modifiers = $price_modifiers;

    $this->base_table = 'rooms_pricing_weekly';
  }

  public function calculatePrice(DateTime $start_date, DateTime $end_date, $persons = 0, $children = 0, $children_ages = array()) {
    $weekly_rates = $this->getRawDayData($start_date, $end_date);

    $daily_calendar = new UnitPricingCalendar($this->unit->unit_id);
    $daily_rates = $daily_calendar->getRawDayData($start_date, $end_date);

    $price = 0;

    foreach ($weekly_rates[$this->unit->unit_id] as $year => $months) {
      foreach ($months as $month => $days) {
        foreach ($days['days'] as $day => $day_price) {
          if ($day_price == 0) {
            $price += $daily_rates[$this->unit->unit_id][$year][$month]['days'][$day] / 100 * 7;
          }
          else {
            $price += $day_price;
          }
        }
      }
    }

    return $price;
  }

  /**
   * {@inheritdoc}
   */
  public function applyPriceModifiers($base_price, $days, &$reply) {
    $price = $base_price;
    if (!empty($this->price_modifiers)) {
      foreach ($this->price_modifiers as $source => $modifier) {

        if ($modifier['#type'] == ROOMS_PRICE_SINGLE_OCCUPANCY) {
          $reply['log']['modifiers'][$source][$mod_count][ROOMS_PRICE_SINGLE_OCCUPANCY]['pre'] = $price;
          $reply['log']['modifiers'][$source][$mod_count][ROOMS_PRICE_SINGLE_OCCUPANCY]['amount'] = $this->unit->data['singlediscount'];
          $reply['log']['modifiers'][$source][$mod_count][ROOMS_PRICE_SINGLE_OCCUPANCY]['modifier'] = $modifier;
            $this->unit->data['singlediscount'];
          $price -= $base_price * $this->unit->data['singlediscount'] / 100;
          $reply['log']['modifiers'][$source][ROOMS_PRICE_SINGLE_OCCUPANCY]['post'] = $price;
        }
        elseif ($modifier['#type'] == ROOMS_DYNAMIC_MODIFIER) {
          $reply['log']['modifiers'][$source][ROOMS_DYNAMIC_MODIFIER]['modifier'] = $modifier;
          $reply['log']['modifiers'][$source][ROOMS_DYNAMIC_MODIFIER]['pre'] = $price;
          switch ($modifier['#op_type']) {
            case ROOMS_ADD:
              $price += $modifier['#amount'] * $modifier['#quantity'];
              $reply['log']['modifiers'][$source][ROOMS_DYNAMIC_MODIFIER]['op'] = ROOMS_ADD;
              $reply['log']['modifiers'][$source][ROOMS_DYNAMIC_MODIFIER]['post'] = $price;
              break;

            case ROOMS_ADD_DAILY:
              $price += $modifier['#amount'] * $modifier['#quantity'] * $days;
              $reply['log']['modifiers'][$source][ROOMS_DYNAMIC_MODIFIER]['op'] = ROOMS_ADD_DAILY;
              $reply['log']['modifiers'][$source][ROOMS_DYNAMIC_MODIFIER]['post'] = $price;
              break;

            case ROOMS_SUB:
              $price -= $modifier['#amount'] * $modifier['#quantity'];
              $reply['log']['modifiers'][$source][ROOMS_DYNAMIC_MODIFIER]['op'] = ROOMS_SUB;
              $reply['log']['modifiers'][$source][ROOMS_DYNAMIC_MODIFIER]['post'] = $price;
              break;

            case ROOMS_SUB_DAILY:
              $price -= $modifier['#amount'] * $modifier['#quantity'] * $days;
              $reply['log']['modifiers'][$source][ROOMS_DYNAMIC_MODIFIER]['op'] = ROOMS_SUB_DAILY;
              $reply['log']['modifiers'][$source][ROOMS_DYNAMIC_MODIFIER]['post'] = $price;
              break;

            case ROOMS_REPLACE:
              $price = $modifier['#amount'];
              $reply['log']['modifiers'][$source][ROOMS_DYNAMIC_MODIFIER]['op'] = ROOMS_REPLACE;
              $reply['log']['modifiers'][$source][ROOMS_DYNAMIC_MODIFIER]['post'] = $price;
              break;

            case ROOMS_INCREASE:
              $price += $base_price * ($modifier['#amount'] * $modifier['#quantity']) / 100;
              $reply['log']['modifiers'][$source][ROOMS_DYNAMIC_MODIFIER]['op'] = ROOMS_INCREASE;
              $reply['log']['modifiers'][$source][ROOMS_DYNAMIC_MODIFIER]['post'] = $price;
              break;

            case ROOMS_DECREASE:
              $price -= $base_price * ($modifier['#amount'] * $modifier['#quantity']) / 100;
              $reply['log']['modifiers'][$source][ROOMS_DYNAMIC_MODIFIER]['op'] = ROOMS_DECREASE;
              $reply['log']['modifiers'][$source][ROOMS_DYNAMIC_MODIFIER]['post'] = $price;
              break;
          }
        }
      }
    }
    return $price;
  }

  /**
   * {@inheritdoc}
   */
  public function getEvents(DateTime $start_date, DateTime $end_date) {
    // Get the raw day results.
    $results = $this->getRawDayData($start_date, $end_date);
    $events = array();
    foreach ($results[$this->unit_id] as $year => $months) {
      foreach ($months as $mid => $month) {
        // Event array gives us the start days for each event within a month.
        $start_days = array_keys($month['states']);
        foreach ($month['states'] as $state) {
          // Create a booking event.
          $start = $state['start_day'];
          $end = $state['end_day'];
          $sd = new DateTime("$year-$mid-$start");
          $ed = new DateTime("$year-$mid-$end");
          $amount = number_format($state['state']);
          $event = new PricingEvent($this->unit_id, $amount, $sd, $ed);
          $events[] = $event;
        }
      }
    }
    return $events;
  }

  /**
   * {@inheritdoc}
   */
  public function getRawDayData(DateTime $start_date, DateTime $end_date) {
    // To handle single-day bookings (Tours) we pretend that they are overnight
    // bookings.
    if ($end_date < $start_date) {
      $end_date->add(new DateInterval('P1D'));
    }

    // Create a dummy PricingEvent to represent the range we are searching over.
    // This gives us access to handy functions that PricingEvents have.
    $s = new PricingEvent($this->unit_id, 0, $start_date, $end_date);

    $results = array();

    // If search across the same year do a single query.
    if ($s->sameYear()) {
      $query = db_select($this->base_table, 'a');
      $query->fields('a');
      $query->condition('a.unit_id', $this->unit_id);
      $query->condition('a.year', $s->startYear());
      $query->condition('a.month', $s->startMonth(), '>=');
      $query->condition('a.month', $s->endMonth(), '<=');
      $months = $query->execute()->fetchAll(PDO::FETCH_ASSOC);
      if (count($months) > 0) {
        foreach ($months as $month) {
          $m = $month['month'];
          $y = $month['year'];
          $id = $month['unit_id'];
          // Remove the three first rows and just keep the days.
          unset($month['month']);
          unset($month['year']);
          unset($month['unit_id']);
          $results[$id][$y][$m]['days'] = $month;
        }
      }
    }
    // For multiple years do a query for each year.
    else {
      for ($j = $s->startYear(); $j <= $s->endYear(); $j++) {
        $query = db_select($this->base_table, 'a');
        $query->fields('a');
        $query->condition('a.unit_id', $this->unit_id);
        $query->condition('a.year', $j);
        if ($j == $s->startYear()) {
          $query->condition('a.month', $s->startMonth(), '>=');
        }
        elseif ($j == $s->endYear()) {
          $query->condition('a.month', $s->endMonth(), '<=');
        }
        $months = $query->execute()->fetchAll(PDO::FETCH_ASSOC);
        if (count($months) > 0) {
          foreach ($months as $month) {
            $m = $month['month'];
            $y = $month['year'];
            $id = $month['unit_id'];
            unset($month['month']);
            unset($month['year']);
            unset($month['unit_id']);
            $results[$id][$y][$m]['days'] = $month;
          }
        }
      }
    }

    $end_of_month_dates = array();

    // With the results from the db in place fill in any missing months
    // with the default state for the unit.
    for ($j = $s->startYear(); $j <= $s->endYear(); $j++) {
      $end_of_month_dates[$j] = rooms_end_of_month_dates($j);

      // We start by setting the expected start and end months for each year.
      if ($s->sameYear()) {
        $expected_months = $s->endMonth() - $s->startMonth() + 1;
        $sm = $s->startMonth();
        $em = $s->endMonth();
      }
      elseif ($j == $s->endYear()) {
        $expected_months = $s->endMonth();
        $sm = 1;
        $em = $s->endMonth();
      }
      elseif ($j == $s->startYear()) {
        $expected_months = 12 - $s->startMonth() + 1;
        $em = 12;
        $sm = $s->startMonth();
      }
      else {
        $expected_months = 12;
        $sm = 1;
        $em = 12;
      }

      // We check to see if the months we have already fit our expectations.
      $actual_months = isset($result[$this->unit_id][$j]) ? count($results[$id][$j]) : 0;
      if ($expected_months > $actual_months) {
        // We have missing months so lets go fill them.
        for ($i = $sm; $i <= $em; $i++) {
          if (!isset($results[$this->unit_id][$j][$i])) {
            $last_day = $end_of_month_dates[$j][$i];
            $month = $this->prepareFullMonthArray(new PricingEvent($this->unit_id,
              $this->default_discount,
              new DateTime("$j-$i-1"),
              new DateTime("$j-$i-$last_day")));
            // Add the month in its rightful position.
            $results[$this->unit_id][$j][$i]['days'] = $month;
            // And sort months.
            ksort($results[$this->unit_id][$j]);
          }
        }
      }
    }

    // With all the months in place we now need to clean results to set the
    // right start and end date for each month - this will save code downstream
    // from having to worry about it.
    foreach ($results[$this->unit_id] as $year => $months) {
      foreach ($months as $mid => $days) {
        // There is undoubtetly a smarter way to do this.
        if (count($days['days']) != $end_of_month_dates[$year][$mid]) {
          switch ($end_of_month_dates[$year][$mid]) {
            case 30:
              unset($results[$this->unit_id][$year][$mid]['days']['d31']);
              break;

            case 29:
              unset($results[$this->unit_id][$year][$mid]['days']['d31']);
              unset($results[$this->unit_id][$year][$mid]['days']['d30']);
              break;

            case 28:
              unset($results[$this->unit_id][$year][$mid]['days']['d31']);
              unset($results[$this->unit_id][$year][$mid]['days']['d30']);
              unset($results[$this->unit_id][$year][$mid]['days']['d29']);
              break;
          }
        }
        if (($year == $s->startYear()) && ($mid == $s->startMonth())) {
          // We know we have the entire months over the range so we just unset
          // all the dates from the start of the month to the actual start day.
          for ($i = 1; $i < $s->startDay(); $i++) {
            unset($results[$this->unit_id][$year][$mid]['days']['d' . $i]);
          }
        }
        if (($year == $s->endYear()) && ($mid == $s->endMonth())) {
          // And from the end of the month back to the actual end day.
          for ($i = $s->endDay() + 1; $i <= $end_of_month_dates[$year][$mid]; $i++) {
            unset($results[$this->unit_id][$year][$mid]['days']['d' . $i]);
          }
        }

      }
    }

    // We store -1 instead of the default price in the DB so this is our chance to get the default price back
    // cycling through the data and replace -1 with the current default price of the unit.
    foreach ($results[$this->unit_id] as $year => $months) {
      foreach ($months as $mid => $days) {
        // The number of days in the month we are interested in eventing.
        $j = count($days);
        // The start date.
        $i = substr(key($days['days']), 1);
        while ($j <= count($days['days'])) {
          if ($days['days']['d' . $i] == -1) {
            $results[$this->unit_id][$year][$mid]['days']['d' . $i] = $this->default_discount;
          }
          $i++;
          $j++;
        }
      }
    }

    // With the results in place we do a states array with the start and
    // end dates of each event.
    foreach ($results[$this->unit_id] as $year => $months) {
      foreach ($months as $mid => $days) {
        // The number of days in the month we are interested in eventing.
        $j = count($days);
        // The start date.
        $i = substr(key($days['days']), 1);
        $start_day = $i;
        $end_day = NULL;
        $unique_states = array();
        $old_state = $days['days']['d' . $i];
        $state = $days['days']['d' . $i];
        while ($j <= count($days['days'])) {
          $state = $days['days']['d' . $i];
          if ($state != $old_state) {
            $unique_states[] = array(
              'state' => $old_state,
              'start_day' => $start_day,
              'end_day' => $i - 1,
            );
            $end_day = $i - 1;
            $start_day = $i;
            $old_state = $state;
          }
          $i++;
          $j++;
        }
        // Get the last event in.
        $unique_states[] = array(
          'state' => $state,
          'start_day' => isset($end_day) ? $end_day + 1 : $start_day,
          'end_day' => $i - 1,
        );
        $results[$this->unit_id][$year][$mid]['states'] = $unique_states;
      }
    }

    return $results;
  }

  /**
   * {@inheritdoc}
   */
  public function updateCalendar($events) {

    foreach ($events as $event) {
      // Make sure event refers to the unit for this calendar.
      if ($event->unit_id == $this->unit_id) {
        // Get all the pricing events that fit within this event.
        $affected_events = $this->getEvents($event->start_date, $event->end_date);
        $monthly_events = array();

        foreach ($affected_events as $a_event) {
          /** @var PricingEventInterface $a_event */
          // Apply the operation.
          $a_event->applyOperation($event->amount, $event->operation);
          // If the event is in the same month span just queue to be added.
          if ($a_event->sameMonth()) {
            $monthly_events[] = $a_event;
          }
          else {
            // Check if multi-year - if not just create monthly events.
            if ($a_event->sameYear()) {
              $monthly_events_tmp = $a_event->transformToMonthlyEvents();
              $monthly_events = array_merge($monthly_events, $monthly_events_tmp);
            }
            else {
              // Else transform to single years and then to monthly.
              $yearly_events = $a_event->transformToYearlyEvents();
              foreach ($yearly_events as $ye) {
                $monthly_events_tmp = $ye->transformToMonthlyEvents();
                $monthly_events = array_merge($monthly_events, $monthly_events_tmp);
              }
            }
          }
        }

        foreach ($monthly_events as $event) {
          $this->addMonthEvent($event);
        }
      }
    }
  }

  /**
   * {@inheritdoc}
   */
  protected function prepareFullMonthArray(RoomsEventInterface $event) {
    $days = array();
    $eod = rooms_end_of_month_dates($event->startYear());
    $last_day = $eod[$event->startMonth()];

    for ($i = 1; $i <= $last_day; $i++) {
      if (($i >= $event->startDay()) && ($i <= $event->endDay())) {
        $days['d' . $i] = $event->amount;
      }
      else {
        // When we are writing a new month to the DB make sure to have the placeholder value -1 for the days where the
        // default price is in effect. This means as a user changes the default price we will take it into account even
        // though the price data is now in a DB row.
        $days['d' . $i] = -1;
      }
    }
    return $days;
  }

  /**
   * {@inheritdoc}
   */
  protected function preparePartialMonthArray(RoomsEventInterface $event) {
    $days = array();
    for ($i = $event->startDay(); $i <= $event->endDay(); $i++) {
      $days['d' . $i] = $event->amount;
    }
    return $days;
  }

  /**
   * {@inheritdoc}
   */
  public function calculatePricingEvents($unit_id, $amount, DateTime $start_date, DateTime $end_date, $operation, $days) {
    $s_timestamp = $start_date->getTimestamp();
    $e_timestamp = $end_date->getTimestamp();

    $events = array();

    do {
      $s_date = getdate($s_timestamp);
      $wday_start = $s_date['wday'];

      if (in_array($wday_start + 1, $days)) {
        $events[] = new PricingEvent($unit_id, $amount, new DateTime(date('Y-m-d', $s_timestamp)), new DateTime(date('Y-m-d', $s_timestamp)), $operation, $days);
      }

      $s_timestamp = strtotime('+1 days', $s_timestamp);

    } while ($s_timestamp <= $e_timestamp);

    return $events;
  }

}
