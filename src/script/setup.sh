#!/bin/sh
# This script creats the shared memory key and binary semaphore keys on tmp folder.
# This script must be once executed before using isc system.
touch /tmp/prod_shmem_key
touch /tmp/prod_sem_key
touch /tmp/cons_shmem_key
touch /tmp/cons_sem_key