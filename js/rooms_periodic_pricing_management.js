(function ($) {

Drupal.behaviors.roomsAvailabilityPrepareForm = {
  attach: function(context) {
    $("form#rooms-filter-month-form select").once('select').change(function() {
      $("form#rooms-filter-month-form").submit();
    });

    $('#edit-select-all').once('select').change(function() {
      var table = $(this).closest('table')[0];
      if (this.options.selectedIndex == 1) {
        $('input[id^="edit-rooms"]:not(:disabled)', table).attr('checked', true);
      }
      else if (this.options.selectedIndex == 2) {
        $('input[id^="edit-rooms"]:not(:disabled)', table).attr('checked', true);
      }
      else if (this.options.selectedIndex == 3) {
        $('input[id^="edit-rooms"]:not(:disabled)', table).attr('checked', false);
      }
    });
  }
};

Drupal.behaviors.roomsPricing = {
  attach: function(context) {

    // Current month is whatever comes through -1 since js counts months starting from 0
    currentMonth = Drupal.settings.roomsUnitManagement.currentMonth - 1;
    currentYear = Drupal.settings.roomsUnitManagement.currentYear;

    // The first month on the calendar
    month1 = currentMonth;
    year1 = currentYear;

    var calendars = [];
    var i = 0;
    for (i=0;i<Drupal.settings.roomsUnitManagement.roomsNumber;i++) {
      calendars[i] = new Array('#calendar' + i, month1, year1);
    }

    events = [];
    var url = Drupal.settings.basePath + '?q=bat/v1/pricing&units=' + Drupal.settings.roomsUnitManagement.roomsId.join() + '&start_date=' + year1 + '-' + (month1+1) + '-01&duration=1M';
    $.ajax({
      url: url,
      success: function(data) {
        events = data['events'];

        $.each(calendars, function(key, value) {
          $(value[0]).fullCalendar('refetchEvents');
        });
      }
    });

    var c = 0;
    $.each(calendars, function(key, value) {
      phpmonth = value[1]+1;

      var unit_id = Drupal.settings.roomsUnitManagement.roomsId[c];
      var unit_name = Drupal.settings.roomsUnitManagement.roomsName[c];

      $(value[0]).once().fullCalendar({
      	schedulerLicenseKey: '0297822786-fcs-1455697240',
      	height: 250,
      	slotWidth: 60,
        editable: false,
        dayNamesShort:[Drupal.t("Sun"), Drupal.t("Mon"), Drupal.t("Tue"), Drupal.t("Wed"), Drupal.t("Thu"), Drupal.t("Fri"), Drupal.t("Sat")],
        monthNames:[Drupal.t("January"), Drupal.t("February"), Drupal.t("March"), Drupal.t("April"), Drupal.t("May"), Drupal.t("June"), Drupal.t("July"), Drupal.t("August"), Drupal.t("September"), Drupal.t("October"), Drupal.t("November"), Drupal.t("December")],
        header: {
          left: '',
          center: '',
          right: ''
        },
        defaultView:'timelineMonth',
        defaultDate: moment([value[2],value[1]]),
        resourceAreaWidth: '12%',
        resourceLabelText: 'Rates for Unit',
        resources: [
          { id: unit_id, title: unit_name, children: [
            { id: 'daily', title: 'Nightly Rate' },
            { id: 'weekly', title: 'Weekly Rate' },
            { id: 'monthly', title: 'Monthly Rate' }
          ] }
        ],
        events: '/rooms/units/unit/' + unit_id + '/roomify-pricing/json',
        //Remove Time from events
        eventRender: function(event, el) {
          el.find('.fc-time').remove();
        },
        eventAfterRender: function(event, element, view) {
          // Append event title when rendering as background.
          if (event.rendering == 'background') {
            var start = event.start.clone();

            if (event.end === null) {
              var end = event.start.clone();
            }
            else {
              var end = event.end.clone();
            }

            var index = 0;

            // Event width.
            var width = element.width()
            // Event colspan number.
            var colspan = element.get(0).colSpan;

            if (event.resourceId == "weekly" || event.resourceId == "monthly") {
              if (event.end != null) {
                // Single cell width.
                var cell_width = width/(end.diff(start, 'days') + 1);

                while (start <= end) {
                  if (event.title == 0) {
                    element.append('<span class="fc-title" style="position:absolute; top:8px; left:' + (index * cell_width + 3) + 'px;">N/A</span>');
                  }
                  else {
                    element.append('<span class="fc-title" style="position:absolute; top:8px; left:' + (index * cell_width + 3) + 'px;">$' + (event.title || '&nbsp;') + '</span>');
                  }
  

                  start = start.add(1, 'day');
                  index++;
                }
              }
              else {
                if (event.title == 0) {
                  element.append('<span class="fc-title" style="position:absolute; top:8px; left:3px;">N/A</span>');
                }
                else {
                  element.append('<span class="fc-title" style="position:absolute; top:8px; left:3px;">$' + (event.title || '&nbsp;') + '</span>');
                }
              }
            }
            else if (event.resourceId == "daily") {
              if (event.end != null) {
                // Single cell width.
                var cell_width = width/(end.diff(start, 'days') + 1);

                while (start <= end) {
                  element.append('<span class="fc-title" style="position:absolute; top:8px; left:' + (index * cell_width + 3) + 'px;">$' + (event.title || '&nbsp;') + '</span>');
                  start = start.add(1, 'day');
                  index++;
                }
              }
              else {
                element.append('<span class="fc-title" style="position:absolute; top:8px; left:3px;">$' + (event.title || '&nbsp;') + '</span>');
                start = start.add(1, 'day');
              }
            }
          }
        }
      });

      c++;
    });

    // Resize takes care of some quirks on occasion
    $(window).resize();

  }
};

})(jQuery);
