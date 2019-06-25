
#include <iostream>

#include <async/jump.hpp>
#include <helix/memory.hpp>
#include <protocols/clock/defs.hpp>
#include <protocols/mbus/client.hpp>
#include <clock.pb.h>

// ----------------------------------------------------------------------------
// RTC handling.
// ----------------------------------------------------------------------------

// Pair of (reference clock, rtc time).
using RtcTime = std::pair<int64_t, int64_t>;

helix::UniqueLane rtcLane;
async::jump foundRtc;

async::result<void> enumerateRtc() {
	auto root = co_await mbus::Instance::global().getRoot();

	auto filter = mbus::Conjunction({
		mbus::EqualsFilter("class", "rtc")
	});
	
	auto handler = mbus::ObserverHandler{}
	.withAttach([] (mbus::Entity entity, mbus::Properties properties) -> async::detached {
		std::cout << "drivers/clocktracker: Found RTC" << std::endl;

		rtcLane = helix::UniqueLane(co_await entity.bind());
		foundRtc.trigger();
	});

	co_await root.linkObserver(std::move(filter), std::move(handler));
	co_await foundRtc.async_wait();
}

async::result<RtcTime> getRtcTime() {
	helix::Offer offer;
	helix::SendBuffer send_req;
	helix::RecvInline recv_resp;

	managarm::clock::CntRequest req;
	req.set_req_type(managarm::clock::CntReqType::RTC_GET_TIME);

	auto ser = req.SerializeAsString();
	auto &&transmit = helix::submitAsync(rtcLane, helix::Dispatcher::global(),
			helix::action(&offer, kHelItemAncillary),
			helix::action(&send_req, ser.data(), ser.size(), kHelItemChain),
			helix::action(&recv_resp));
	co_await transmit.async_wait();
	HEL_CHECK(offer.error());
	HEL_CHECK(send_req.error());
	HEL_CHECK(recv_resp.error());

	managarm::clock::SvrResponse resp;
	resp.ParseFromArray(recv_resp.data(), recv_resp.length());
	assert(resp.error() == managarm::clock::Error::SUCCESS);
	
	co_return RtcTime{resp.ref_nanos(), resp.time_nanos()};
}

// ----------------------------------------------------------------------------
// Tracker page handling.
// ----------------------------------------------------------------------------

helix::UniqueDescriptor trackerPageMemory;
helix::Mapping trackerPageMapping;

TrackerPage *accessPage() {
	return reinterpret_cast<TrackerPage *>(trackerPageMapping.get());
}

// ----------------------------------------------------------------------------
// clocktracker mbus interface.
// ----------------------------------------------------------------------------

async::detached serve(helix::UniqueLane lane) {
	while(true) {
		helix::Accept accept;
		helix::RecvInline recv_req;

		auto &&header = helix::submitAsync(lane, helix::Dispatcher::global(),
				helix::action(&accept, kHelItemAncillary),
				helix::action(&recv_req));
		co_await header.async_wait();
		HEL_CHECK(accept.error());
		HEL_CHECK(recv_req.error());
		
		auto conversation = accept.descriptor();

		managarm::clock::CntRequest req;
		req.ParseFromArray(recv_req.data(), recv_req.length());
		if(req.req_type() == managarm::clock::CntReqType::ACCESS_PAGE) {
			helix::SendBuffer send_resp;
			helix::PushDescriptor send_memory;

			managarm::clock::SvrResponse resp;
			resp.set_error(managarm::clock::Error::SUCCESS);

			auto ser = resp.SerializeAsString();
			auto &&transmit = helix::submitAsync(conversation, helix::Dispatcher::global(),
					helix::action(&send_resp, ser.data(), ser.size(), kHelItemChain),
					helix::action(&send_memory, trackerPageMemory));
			co_await transmit.async_wait();
			HEL_CHECK(send_resp.error());
		}else{
			throw std::runtime_error("Unexpected request type");
		}
	}
}

// ----------------------------------------------------------------
// Freestanding mbus functions.
// ----------------------------------------------------------------

async::detached initializeDriver() {
	// Find an RTC on the mbus.
	co_await enumerateRtc();

	// Allocate and map our tracker page.
	size_t page_size = 4096;
	HelHandle handle;
	HEL_CHECK(helAllocateMemory(page_size, 0, &handle));
	trackerPageMemory = helix::UniqueDescriptor{handle};
	trackerPageMapping = helix::Mapping{trackerPageMemory, 0, page_size};

	// Initialize the tracker page.
	auto page = accessPage();
	memset(page, 0, page_size);

	// Read the RTC to initialize the realtime clock.
	auto result = co_await getRtcTime(); // TODO: Use the seqlock.
	std::cout << "drivers/clocktracker: Initializing time to "
			<< std::get<1>(result) << std::endl;
	accessPage()->refClock = std::get<0>(result);
	accessPage()->baseRealtime = std::get<1>(result);

	// Create an mbus object for the device.
	auto root = co_await mbus::Instance::global().getRoot();
	
	mbus::Properties descriptor{
		{"class", mbus::StringItem{"clocktracker"}},
	};

	auto handler = mbus::ObjectHandler{}
	.withBind([=] () -> async::result<helix::UniqueDescriptor> {
		helix::UniqueLane local_lane, remote_lane;
		std::tie(local_lane, remote_lane) = helix::createStream();
		serve(std::move(local_lane));

		async::promise<helix::UniqueDescriptor> promise;
		promise.set_value(std::move(remote_lane));
		return promise.async_get();
	});

	co_await root.createObject("clocktracker", descriptor, std::move(handler));
}

int main() {
	std::cout << "drivers/clocktracker: Starting driver" << std::endl;

	{
		async::queue_scope scope{helix::globalQueue()};
		initializeDriver();
	}

	helix::globalQueue()->run();
	
	return 0;
}


