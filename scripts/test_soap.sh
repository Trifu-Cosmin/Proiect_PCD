#!/bin/bash

URL="http://127.0.0.1:8082/soap"

call_soap() {
  OPERATION="$1"

  echo
  echo "SOAP $OPERATION"

  curl -s -X POST "$URL" \
    -H "Content-Type: text/xml; charset=utf-8" \
    -d "<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<soap:Envelope xmlns:soap=\"http://schemas.xmlsoap.org/soap/envelope/\">
  <soap:Body>
    <$OPERATION/>
  </soap:Body>
</soap:Envelope>"
  echo
}

echo "Testing SOAP API..."

call_soap "Health"
call_soap "Stats"
call_soap "Reports"
call_soap "Uploads"
call_soap "Jobs"
call_soap "Users"
call_soap "Logs"

echo
echo "SOAP API test finished."