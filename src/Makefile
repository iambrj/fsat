builder:
	clang++ builder.cpp -o builder

host:
	clang++ host.cpp -o host

create_kernal:
	./builder ${INPUT_FILE}

kernal: builder create_kernal

clean:
	rm -f builder host kernal.c
