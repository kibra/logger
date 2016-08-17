package main

import "time"
import "fmt"

import "github.com/kibra/logger"
import "github.com/uber-go/zap"

func main() {
	log := zap.New(
		zap.NewJSONEncoder(zap.RFC3339Formatter("ts")), // drop timestamps in tests
		zap.OutputNoSync(logger.NewLoggerWriter()),
	)

	i := 0
	for {
		log.Warn(fmt.Sprintf("Log without structured data %d...", i))
		i++
		time.Sleep(100 * time.Millisecond)



	}

	time.Sleep(1000 * time.Millisecond)
	log.Warn("Log without struct..")
}