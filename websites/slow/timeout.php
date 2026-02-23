<?php

// Send header immediately
echo "Content-Type: text/plain\r\n\r\n";

// Flush headers early
flush();

// Simulate heavy processing
sleep(10);

echo "Finished after 5 seconds\n";
