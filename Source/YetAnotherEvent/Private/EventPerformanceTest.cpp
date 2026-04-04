#include "Misc/AutomationTest.h"
#include "../Event/EventSystem.h"

// Standard multicast delegate for comparison
DECLARE_MULTICAST_DELEGATE_OneParam(FStandardUnrealEvent, int32);

// Register the test under the category "YetAnotherEvent -> Events -> Benchmark"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEventPerformanceTest, "YetAnotherEvent.Events.Benchmark", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEventPerformanceTest::RunTest(const FString& Parameters)
{
	constexpr int32 NUM_LISTENERS = 100;
	constexpr int32 NUM_BROADCASTS = 1000000; // 1 million broadcasts
	int32 DummyCounter = 0;

	// Setup
	TEvent<int32> CustomEvent;
	FStandardUnrealEvent UnrealEvent;

	// Subscribe 100 listeners to each event
	for (int32 i = 0; i < NUM_LISTENERS; ++i) {
		CustomEvent.SubscribeLambda([&DummyCounter](int32 Value) {
			DummyCounter += Value;
			});
	}

	for (int32 i = 0; i < NUM_LISTENERS; ++i) {
		UnrealEvent.AddLambda([&DummyCounter](int32 Value) {
			DummyCounter += Value;
			});
	}

	// Benchmark 1: Unreal event
	const double UNREAL_START_TIME = FPlatformTime::Seconds();
	for (int32 i = 0; i < NUM_BROADCASTS; ++i) {
		UnrealEvent.Broadcast(1);
	}
	const double UNREAL_END_TIME = FPlatformTime::Seconds();
	const double UNREAL_TOTAL_TIME_MS = (UNREAL_END_TIME - UNREAL_START_TIME) * 1000.0f;

	// Benchmark 2: Our event
	const double CUSTOM_START_TIME = FPlatformTime::Seconds();
	for (int32 i = 0; i < NUM_BROADCASTS; ++i) {
		CustomEvent.Broadcast(1);
	}
	const double CUSTOM_END_TIME = FPlatformTime::Seconds();
	const double CUSTOM_TOTAL_TIME_MS = (CUSTOM_END_TIME - CUSTOM_START_TIME) * 1000.0f;

	const double CUSTOM_TIME_PER_BROADCAST_MS = CUSTOM_TOTAL_TIME_MS / (double)NUM_BROADCASTS;
	const double UNREAL_TIME_PER_BROADCAST_MS = UNREAL_TOTAL_TIME_MS / (double)NUM_BROADCASTS;

	const int32 CUSTOM_BROADCASTS_PER_MS = FMath::FloorToInt32(1.0f / CUSTOM_TIME_PER_BROADCAST_MS);
	const int32 UNREAL_BROADCASTS_PER_MS = FMath::FloorToInt32(1.0f / UNREAL_TIME_PER_BROADCAST_MS);

	// Output the results
	UE_LOG(LogTemp, Warning, TEXT("<=======> EVENT PERFORMANCE BENCHMARK <=======>"));
	UE_LOG(LogTemp, Display, TEXT("Unreal Delegate: %f ms TOTAL"), UNREAL_TOTAL_TIME_MS);
	UE_LOG(LogTemp, Display, TEXT("TEvent:          %f ms TOTAL"), CUSTOM_TOTAL_TIME_MS);
	UE_LOG(LogTemp, Display, TEXT("-----------------------------------------------"));

	UE_LOG(LogTemp, Display, TEXT("TIME PER BROADCAST (100 Listeners):"));
	UE_LOG(LogTemp, Display, TEXT("Unreal Delegate: %f ms"), UNREAL_TIME_PER_BROADCAST_MS);
	UE_LOG(LogTemp, Display, TEXT("TEvent:          %f ms"), CUSTOM_TIME_PER_BROADCAST_MS);
	UE_LOG(LogTemp, Display, TEXT("-----------------------------------------------"));

	UE_LOG(LogTemp, Display, TEXT("BUDGET PER 1 MILLISECOND OF FRAME TIME:"));
	UE_LOG(LogTemp, Display, TEXT("Unreal Delegate: %d Broadcasts per 1ms"), UNREAL_BROADCASTS_PER_MS);
	UE_LOG(LogTemp, Display, TEXT("TEvent:          %d Broadcasts per 1ms"), CUSTOM_BROADCASTS_PER_MS);
	UE_LOG(LogTemp, Display, TEXT("-----------------------------------------------"));

	if (CUSTOM_TOTAL_TIME_MS < UNREAL_TOTAL_TIME_MS) {
		UE_LOG(LogTemp, Display, TEXT("Custom TEvent is FASTER by %f ms!"), UNREAL_TOTAL_TIME_MS - CUSTOM_TOTAL_TIME_MS);
		UE_LOG(LogTemp, Display, TEXT("TEvent is faster by %f%%!"), (UNREAL_TOTAL_TIME_MS / CUSTOM_TOTAL_TIME_MS) * 100.0f);
	}
	else {
		UE_LOG(LogTemp, Display, TEXT("Custom TEvent is SLOWER by %f ms!"), CUSTOM_TOTAL_TIME_MS - UNREAL_TOTAL_TIME_MS);
		UE_LOG(LogTemp, Display, TEXT("TEvent is slower by %f%%!"), (CUSTOM_TOTAL_TIME_MS / UNREAL_TOTAL_TIME_MS) * 100.0f);
	}
	UE_LOG(LogTemp, Warning, TEXT("<=======> EVENT PERFORMANCE BENCHMARK <=======>"));

	return true; // Test ran successfully
}