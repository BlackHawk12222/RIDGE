import asyncio

def run(coro):
    loop = asyncio.get_event_loop()
    loop.run_until_complete(coro)

def create_task(coro):
    loop = asyncio.get_event_loop()
    return loop.create_task(coro)

def sleep_ms(ms):
    loop = asyncio.get_event_loop()
    return loop.run_in_executor(None, asyncio.sleep, ms / 1000)