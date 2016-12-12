api = 2
core = 7.x

; @see https://github.com/drush-ops/drush

; FullCalendar Scheduler
libraries[scheduler][directory_name] = fullcalendar-scheduler
libraries[scheduler][type] = library
libraries[scheduler][destination] = libraries
libraries[scheduler][download][type] = get
libraries[scheduler][download][url] = https://github.com/fullcalendar/fullcalendar-scheduler/releases/download/v1.5.0/fullcalendar-scheduler-1.5.0.zip
