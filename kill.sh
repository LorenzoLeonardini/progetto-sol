#!/bin/bash

kill -9 $(ps aux | grep supermercato | awk '{print $2}')
