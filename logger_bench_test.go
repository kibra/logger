package main

import (
	"testing"
)

// import "time"

import "github.com/kibra/logger"



func BenchmarkLoggerCgo(b *testing.B) {
	log := logger.NewLoggerWriter()
	log_msg := []byte("Blah blah blah")
	for i := 0; i < b.N; i++ {
		log.WriteCgo(log_msg)
		// time.Sleep(1 * time.Millisecond)

	}
	b.ReportAllocs()
}

func BenchmarkLoggerNativeGo(b *testing.B) {
	log := logger.NewLoggerWriter()
	log_msg := []byte("Blah blah blah")
	for i := 0; i < b.N; i++ {
		log.Write(log_msg)
		// time.Sleep(1 * time.Millisecond)
	}
	b.ReportAllocs()
}

func BenchmarkLoggerNativeNoMagicNoReflect(b *testing.B) {
	log := logger.NewLoggerWriter()
	log_msg := []byte("Blah blah blah")
	for i := 0; i < b.N; i++ {
		log.WriteNoCrazyNoReflect(log_msg)
		// time.Sleep(1 * time.Millisecond)
	}
	b.ReportAllocs()
}
