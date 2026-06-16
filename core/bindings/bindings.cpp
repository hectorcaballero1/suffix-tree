#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "suffix_tree.hpp"
#include "corpus_tree.hpp"
#include "naive_search.hpp"

namespace py = pybind11;

PYBIND11_MODULE(suffix_tree_core, m) {
    m.doc() = "Ukkonen suffix tree + plagiarism detection core (C++)";

    py::class_<SuffixTree>(m, "SuffixTree")
        .def(py::init<>())
        .def("build",             &SuffixTree::build)
        .def("contains",          &SuffixTree::contains)
        .def("count_occurrences", &SuffixTree::count_occurrences)
        .def("find_occurrences",  &SuffixTree::find_occurrences)
        .def("get_tree_path",     &SuffixTree::get_tree_path);

    py::class_<Occurrence>(m, "Occurrence")
        .def_readonly("position", &Occurrence::position)
        .def_readonly("source",   &Occurrence::source);

    py::class_<CorpusTree>(m, "CorpusTree")
        .def(py::init<>())
        .def("build",            &CorpusTree::build)
        .def("contains",         &CorpusTree::contains)
        .def("find_occurrences", &CorpusTree::find_occurrences);

    m.def("naive_search", &naive_search,
          py::arg("text"), py::arg("pattern"));
}
