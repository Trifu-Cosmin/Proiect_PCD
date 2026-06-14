#!/bin/bash

BASE_URL="http://127.0.0.1:8081"

echo "Testing REST API..."

echo
echo "GET /health"
curl -s "$BASE_URL/health"
echo

echo
echo "GET /stats"
curl -s "$BASE_URL/stats"
echo

echo
echo "GET /reports"
curl -s "$BASE_URL/reports"
echo

echo
echo "GET /uploads"
curl -s "$BASE_URL/uploads"
echo

echo
echo "GET /jobs"
curl -s "$BASE_URL/jobs"
echo

echo
echo "GET /users"
curl -s "$BASE_URL/users"
echo

echo
echo "GET /logs"
curl -s "$BASE_URL/logs"
echo

echo
echo "REST API test finished."