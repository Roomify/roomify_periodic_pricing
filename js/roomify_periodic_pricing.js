(function ($) {

// define object
Drupal.RoomifyPricing = Drupal.RoomifyPricing || {};
Drupal.RoomifyPricing.Modal = Drupal.RoomifyPricing.Modal || {};

Drupal.behaviors.roomify_pricing = {
  attach: function(context) {

    unit_id = Drupal.settings.roomsPeriodicPricing.roomID;
    unit_name = Drupal.settings.roomsPeriodicPricing.roomName;

    // Current month is whatever comes through -1 since js counts months starting
    // from 0
    currentMonth = Drupal.settings.roomsCalendar.currentMonth - 1;
    currentYear = Drupal.settings.roomsCalendar.currentYear;

    // The first month on the calendar
    month1 = currentMonth;
    year1 = currentYear;

    var calendars = [];
    calendars[0] = new Array('#calendar', month1, year1);

    $(document).one('CToolsDetachBehaviors', function(e, modal) {
      var date = $(modal).find('input[name="rooms_start_date[date]"]').val();
      if (date != undefined) {
        $('#calendar').fullCalendar('next');
        $('#calendar').fullCalendar('gotoDate', date);
      }
    });

    $.each(calendars, function(key, value) {
      $(value[0]).once().fullCalendar({
        schedulerLicenseKey: '0297822786-fcs-1455697240',
        height: 400,
        slotWidth: 60,
        editable: false,
        selectable: true,
        dayNamesShort:[Drupal.t("Sun"), Drupal.t("Mon"), Drupal.t("Tue"), Drupal.t("Wed"), Drupal.t("Thu"), Drupal.t("Fri"), Drupal.t("Sat")],
        monthNames:[Drupal.t("January"), Drupal.t("February"), Drupal.t("March"), Drupal.t("April"), Drupal.t("May"), Drupal.t("June"), Drupal.t("July"), Drupal.t("August"), Drupal.t("September"), Drupal.t("October"), Drupal.t("November"), Drupal.t("December")],
        header: {
          left: 'today, prev, next',
          center: 'title',
          right: 'timelineThirtyDay, timelineMonth, timeline365Day',
        },
        views: {
          timelineThirtyDay: {
            buttonText: '30 days',
            duration: { days: 30 },
            type: 'timeline',
          },
          timeline365Day: {
            buttonText: 'year',
            duration: { days: 365 },
            type: 'timeline',
          }
        },
        defaultView: 'timeline365Day',
        resourceAreaWidth: '15%',
        resourceLabelText: 'Rates for Unit',
        resources: [
          { id: unit_id + ' availability', title: unit_name + ' Availability', children: [
            { id: 'availability', title: 'Availability' }
          ] },
          { id: unit_id + ' rates', title: unit_name + ' Rates', children: [
            { id: 'nightly', title: 'Nightly Rate' },
            { id: 'weekly', title: 'Weekly Rate' },
            { id: 'monthly', title: 'Monthly Rate' }
          ] }
        ],
        events: '/rooms/units/unit/' + unit_id + '/roomify-pricing/json',
        windowResize: function(view) {
          $(this).fullCalendar('refetchEvents');
        },
        select: function(start, end, jsEvent, view, resource) {
          var type = resource.id;

          var ed = end.subtract(1, 'days');
          var sd = start.format('YYYY-MM-DD');
          ed = end.format('YYYY-MM-DD');

          // Open the modal for edit
          Drupal.RoomifyPricing.Modal(this, type, sd, ed);
          $(value[0]).fullCalendar('unselect');
        },
        // Remove Time from events
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
            else if (event.resourceId == "nightly") {
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
    });
    // Resize takes care of some quirks on occasion
    $(window).resize();
  }
};

/**
 * Initialize the modal box.
 */
Drupal.RoomifyPricing.Modal = function(element, eid, sd, ed) {
  // base url the part that never change is used to identify our ajax instance
  var base = Drupal.settings.basePath + '?q=admin/rooms/units/unit/';

  if (eid == 'availability') {
    Drupal.CTools.Modal.show('rooms-modal-style');

    var element_settings = {
      url : base + Drupal.settings.roomsPeriodicPricing.roomID + '/event/' + eid + '/' + sd + '/' + ed,
      event : 'getResponse',
      progress : { type: 'throbber' }
    };
  }
  else if (eid == 'manage availability') {
    Drupal.CTools.Modal.show('rooms-modal-style');

    var element_settings = {
      url : base + Drupal.settings.roomsPeriodicPricing.roomID + '/manage-availability',
      event : 'getResponse',
      progress : { type: 'throbber' }
    };
  }
  else if (eid == 'manage nightly' || eid == 'manage weekly' || eid == 'manage monthly') {
    Drupal.CTools.Modal.show('roomify-modal-style');

    var element_settings = {
      url : base + Drupal.settings.roomsPeriodicPricing.roomID + '/manage-pricing/' + eid.substring(7),
      event : 'getResponse',
      progress : { type: 'throbber' }
    };
  }
  else {
    Drupal.CTools.Modal.show('rooms-modal-style');

    var element_settings = {
      url : base + Drupal.settings.roomsPeriodicPricing.roomID + '/roomify-pricing/' + eid + '/' + sd + '/' + ed,
      event : 'getResponse',
      progress : { type: 'throbber' }
    };
  }

  // To made all calendars trigger correctly the getResponse event we need to
  // initialize the ajax instance with the global calendar table element.
  var calendars_table = $(element.el).closest('.calendar-set');

  if (element.el == undefined) {
    calendars_table = $(element).closest('.calendar-set');
  }

  // create new instance only once if exists just override the url
  if (Drupal.ajax[base] === undefined) {
    Drupal.ajax[base] = new Drupal.ajax(element_settings.url, calendars_table, element_settings);
  }
  else {
    Drupal.ajax[base].element_settings.url = element_settings.url;
    Drupal.ajax[base].options.url = element_settings.url;
  }
  // We need to trigger manually the AJAX getResponse due fullcalendar select
  // event is not recognized by Drupal AJAX
  $(calendars_table).trigger('getResponse');
};

$(document).ready(function () {
  $(document).on('click', '.fc-resource-area tr', function () {
    var resourceId = $(this).attr('data-resource-id');
    if (resourceId == 'nightly' || resourceId == 'weekly' || resourceId == 'monthly' || resourceId == 'availability') {
      Drupal.RoomifyPricing.Modal(this, 'manage ' + resourceId);
    }
  });

  $(document).on('mouseenter', '.fc-resource-area tr', function () {
    var resourceId = $(this).attr('data-resource-id');
    if (resourceId == 'nightly' || resourceId == 'weekly' || resourceId == 'monthly' || resourceId == 'availability') {
      $(this).toggleClass('show-edit');
    }
  });

  $(document).on('mouseleave', '.fc-resource-area tr', function () {
    var resourceId = $(this).attr('data-resource-id');
    if (resourceId == 'nightly' || resourceId == 'weekly' || resourceId == 'monthly' || resourceId == 'availability') {
      $(this).toggleClass('show-edit');
    }
  });
});

})(jQuery);
