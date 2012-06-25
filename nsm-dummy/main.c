/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <glib.h>
#include <gio/gio.h>

#include <dlt/dlt.h>

#include <nsm-dummy/nsm-consumer-service.h>
#include <nsm-dummy/nsm-dummy-application.h>
#include <nsm-dummy/nsm-lifecycle-control-service.h>



DLT_DECLARE_CONTEXT (nsm_dummy_context);



int
main (int    argc,
      char **argv)
{
  NSMLifecycleControlService *lifecycle_control_service;
  NSMDummyApplication        *application;
  NSMConsumerService         *consumer_service;
  GDBusConnection            *connection;
  GError                     *error = NULL;
  int                         exit_status;

  /* register the application and context in DLT */
  DLT_REGISTER_APP ("NSMD", "GENIVI Node State Manager Dummy");
  DLT_REGISTER_CONTEXT (nsm_dummy_context, "NSMC",
                        "Context of the node state manager dummy itself");

  /* initialize the GType type system */
  g_type_init ();

  /* attempt to connect to D-Bus */
  connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
  if (connection == NULL)
    {
      g_warning ("Failed to connect to D-Bus: %s", error->message);

      /* clean up */
      g_error_free (error);

      return EXIT_FAILURE;
    }

  /* instantiate the NSMLifecycleControlService implementation */
  lifecycle_control_service = nsm_lifecycle_control_service_new (connection);
  if (!nsm_lifecycle_control_service_start (lifecycle_control_service, &error))
    {
      g_warning ("Failed to start the LifecycleControl service: %s", error->message);

      /* clean up */
      g_error_free (error);
      g_object_unref (lifecycle_control_service);
      g_object_unref (connection);

      return EXIT_FAILURE;
    }

  /* instantiate the NSMConsumerService implementation */
  consumer_service = nsm_consumer_service_new (connection);
  if (!nsm_consumer_service_start (consumer_service, &error))
    {
      g_warning ("Failed to start the Consumer service: %s", error->message);

      /* clean up */
      g_error_free (error);
      g_object_unref (consumer_service);
      g_object_unref (connection);

      return EXIT_FAILURE;
    }

  /* create and run the main application */
  application = nsm_dummy_application_new (connection, 
                                           consumer_service, 
                                           lifecycle_control_service);
  exit_status = g_application_run (G_APPLICATION (application), 0, NULL);

  /* release allocated objects */
  g_object_unref (application);
  g_object_unref (lifecycle_control_service);
  g_object_unref (consumer_service);
  g_object_unref (connection);

  /* unregister the application and context with DLT */
  DLT_UNREGISTER_CONTEXT (nsm_dummy_context);
  DLT_UNREGISTER_APP ();

  return exit_status;
}
