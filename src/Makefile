builder:
	clang++ builder.cpp -o builder

create_kernal:
	./builder ${INPUT_FILE}

kernal: builder create_kernal

kernal_test:
	clang++ kernal_test.cpp kernal.cpp -o kernal_test

naive:
	clang++ naive.cpp -o naive

clean:
	rm -f builder host kernal_test naive
