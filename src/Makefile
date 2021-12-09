builder:
	clang++ builder.cpp -o builder

create_kernal:
	./builder ${INPUT_FILE}

kernal: builder create_kernal

kernal_test:
	clang++ -O3 kernal_test.cpp kernal.cpp -o kernal_test

naive:
	clang++ -O3 naive.cpp -o naive

testing: kernal_test naive

clean:
	rm -f builder host kernal_test naive
